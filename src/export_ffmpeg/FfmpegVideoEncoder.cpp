#include "export_ffmpeg/FfmpegVideoEncoder.hpp"

#include <atomic>
#include <condition_variable>
#include <deque>
#include <limits>
#include <mutex>
#include <stdexcept>
#include <thread>

#include "platform/ChildProcess.hpp"

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#else
#include <fcntl.h>
#include <unistd.h>
#endif

namespace {
bool active(VideoEncoderState state)
{
  return state == VideoEncoderState::Starting || state == VideoEncoderState::Running ||
         state == VideoEncoderState::Finalizing;
}
std::string utf8(const std::filesystem::path& path)
{
  const auto text = path.u8string();
  return {reinterpret_cast<const char*>(text.data()), text.size()};
}
// Reserve a job-owned filename in the destination directory. Never remove a
// preexisting file or publish across filesystems. The final move refuses races.
std::filesystem::path reserveTemporary(const std::filesystem::path& destination)
{
  static std::atomic_uint64_t serial{0};
  for (int attempt = 0; attempt < 100; ++attempt) {
    auto temporary = destination;
    temporary += ".keywave-" + std::to_string(
      std::chrono::steady_clock::now().time_since_epoch().count()) + "-" +
      std::to_string(++serial) + ".partial.mp4";
#ifdef _WIN32
    const auto file = CreateFileW(temporary.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_NEW,
                                  FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file != INVALID_HANDLE_VALUE) { CloseHandle(file); return temporary; }
    if (GetLastError() != ERROR_FILE_EXISTS && GetLastError() != ERROR_ALREADY_EXISTS) {
      throw std::system_error(static_cast<int>(GetLastError()), std::system_category(),
                              "Cannot create temporary export file");
    }
#else
    const auto file = open(temporary.c_str(), O_WRONLY | O_CREAT | O_EXCL, 0600);
    if (file >= 0) { close(file); return temporary; }
    if (errno != EEXIST) {
      throw std::system_error(errno, std::generic_category(), "Cannot create temporary export file");
    }
#endif
  }
  throw std::runtime_error("Cannot reserve a temporary export filename");
}
void publish(const std::filesystem::path& temporary, const std::filesystem::path& destination)
{
#ifdef _WIN32
  if (!MoveFileExW(temporary.c_str(), destination.c_str(), MOVEFILE_WRITE_THROUGH)) {
    throw std::system_error(static_cast<int>(GetLastError()), std::system_category(),
                            "Cannot publish video (destination must not exist)");
  }
#else
  if (link(temporary.c_str(), destination.c_str()) != 0) {
    throw std::system_error(errno, std::generic_category(), "Cannot publish video");
  }
  std::filesystem::remove(temporary);
#endif
}
} // namespace

struct FfmpegVideoEncoder::Impl
{
  explicit Impl(std::filesystem::path path) : executable(std::move(path)) {}
  std::filesystem::path executable;
  mutable std::mutex mutex;
  std::condition_variable wake;
  std::deque<std::vector<std::uint8_t>> queue;
  std::atomic_bool cancelled{false};
  std::thread worker;
  VideoEncoderStatus status;
  VideoEncoderConfig config;
  std::size_t frameBytes = 0;
  std::uint64_t submitted = 0;
  bool finishing = false;

  void run(const std::filesystem::path& destination)
  {
    std::filesystem::path temporary;
    std::unique_ptr<ChildProcess> process;
    try {
      const auto path = executable.empty() ? ChildProcess::findExecutable("ffmpeg") :
                                             std::filesystem::absolute(executable);
      {
        ChildProcess probe(path, {"-hide_banner", "-h", "encoder=libx264"});
        probe.closeInput();
        if (probe.wait(cancelled, std::chrono::seconds(10)) != 0 ||
            probe.diagnostics().find("Encoder libx264") == std::string::npos) {
          throw std::runtime_error("FFmpeg requires the libx264 H.264 encoder. " + probe.diagnostics());
        }
      }
      if (cancelled) { throw std::runtime_error("Export cancelled"); }
      temporary = reserveTemporary(destination);
      process = std::make_unique<ChildProcess>(path, std::vector<std::string>{
        "-hide_banner", "-loglevel", "error", "-nostdin", "-y", "-f", "rawvideo",
        "-pixel_format", "rgba", "-video_size",
        std::to_string(config.size.width) + "x" + std::to_string(config.size.height),
        "-framerate", std::to_string(config.fps), "-i", "pipe:0", "-an", "-c:v", "libx264",
        "-preset", "medium", "-crf", "18", "-pix_fmt", "yuv420p", "-movflags", "+faststart",
        "-f", "mp4", utf8(temporary)});
      {
        std::lock_guard lock(mutex);
        status.state = VideoEncoderState::Running;
      }
      for (;;) {
        std::vector<std::uint8_t> frame;
        {
          std::unique_lock lock(mutex);
          wake.wait(lock, [this] { return cancelled || finishing || !queue.empty(); });
          if (cancelled) { throw std::runtime_error("Export cancelled"); }
          if (queue.empty() && finishing) { break; }
          frame = std::move(queue.front());
          queue.pop_front();
        }
        process->writeInput(frame, cancelled);
        std::lock_guard lock(mutex);
        ++status.framesWritten;
      }
      process->closeInput();
      if (process->wait(cancelled) != 0) {
        throw std::runtime_error("FFmpeg failed to finalize the MP4");
      }
      if (cancelled) { throw std::runtime_error("Export cancelled"); }
      if (!std::filesystem::is_regular_file(temporary) || std::filesystem::file_size(temporary) == 0) {
        throw std::runtime_error("FFmpeg produced no video");
      }
      // Serialize publication with cancel(): cancellation before publication
      // wins; cancellation after successful publication leaves Completed.
      std::lock_guard lock(mutex);
      if (cancelled) { throw std::runtime_error("Export cancelled"); }
      publish(temporary, destination);
      status.state = VideoEncoderState::Completed;
    }
    catch (const std::exception& error) {
      std::string message = error.what();
      if (process) {
        message += "\n" + process->diagnostics();
        process.reset(); // Kill/reap before removing the job's temporary file.
      }
      std::error_code cleanupError;
      if (!temporary.empty()) { std::filesystem::remove(temporary, cleanupError); }
      std::lock_guard lock(mutex);
      queue.clear();
      status.state = cancelled ? VideoEncoderState::Cancelled : VideoEncoderState::Failed;
      status.error = cancelled ? std::string{} : std::move(message);
      if (cleanupError) { status.error += "\nCannot remove temporary export: " + cleanupError.message(); }
    }
  }
};
FfmpegVideoEncoder::FfmpegVideoEncoder(std::filesystem::path executable)
  : m_impl(std::make_unique<Impl>(std::move(executable))) {}
FfmpegVideoEncoder::~FfmpegVideoEncoder()
{
  cancel();
  if (m_impl->worker.joinable()) { m_impl->worker.join(); }
}
void FfmpegVideoEncoder::start(VideoEncoderConfig config, const std::filesystem::path& destination)
{
  if (active(status().state)) { throw std::logic_error("A video export is already running"); }
  if (config.size.width <= 0 || config.size.height <= 0 || config.size.width % 2 != 0 ||
      config.size.height % 2 != 0 || (config.fps != 30 && config.fps != 60) || config.frameCount == 0) {
    throw std::invalid_argument("Invalid video encoder profile");
  }
  const auto width = static_cast<std::size_t>(config.size.width);
  const auto height = static_cast<std::size_t>(config.size.height);
  if (width > std::numeric_limits<std::size_t>::max() / 4 / height) {
    throw std::overflow_error("Encoder frame buffer is too large");
  }
  const auto output = std::filesystem::absolute(destination);
  if (destination.empty() || !std::filesystem::is_directory(output.parent_path()) ||
      std::filesystem::exists(output)) {
    throw std::invalid_argument("Select a new video file in an existing directory");
  }
  if (m_impl->worker.joinable()) { m_impl->worker.join(); }
  {
    std::lock_guard lock(m_impl->mutex);
    m_impl->queue.clear();
    m_impl->status = {VideoEncoderState::Starting, 0, {}};
    m_impl->config = config;
    m_impl->frameBytes = width * height * 4;
    m_impl->submitted = 0;
    m_impl->finishing = false;
    m_impl->cancelled = false;
  }
  try { m_impl->worker = std::thread([this, output] { m_impl->run(output); }); }
  catch (...) {
    std::lock_guard lock(m_impl->mutex);
    m_impl->status.state = VideoEncoderState::Failed;
    m_impl->status.error = "Cannot start video export worker";
    throw;
  }
}
VideoEncoderStatus FfmpegVideoEncoder::status() const
{
  std::lock_guard lock(m_impl->mutex);
  return m_impl->status;
}
bool FfmpegVideoEncoder::canAcceptFrame() const
{
  std::lock_guard lock(m_impl->mutex);
  return m_impl->status.state == VideoEncoderState::Running && !m_impl->cancelled &&
         !m_impl->finishing && m_impl->submitted < m_impl->config.frameCount &&
         m_impl->queue.size() < queueCapacity;
}
bool FfmpegVideoEncoder::trySubmit(std::vector<std::uint8_t>&& rgba)
{
  std::lock_guard lock(m_impl->mutex);
  if (m_impl->status.state != VideoEncoderState::Running || m_impl->cancelled ||
      m_impl->finishing || m_impl->queue.size() >= queueCapacity ||
      m_impl->submitted >= m_impl->config.frameCount) { return false; }
  if (rgba.size() != m_impl->frameBytes) { throw std::invalid_argument("Incorrect RGBA frame size"); }
  m_impl->queue.push_back(std::move(rgba));
  ++m_impl->submitted;
  m_impl->wake.notify_one();
  return true;
}
void FfmpegVideoEncoder::finish()
{
  std::lock_guard lock(m_impl->mutex);
  if (m_impl->status.state != VideoEncoderState::Running ||
      m_impl->submitted != m_impl->config.frameCount) {
    throw std::logic_error("Export cannot finalize before all frames have been submitted");
  }
  m_impl->finishing = true;
  m_impl->status.state = VideoEncoderState::Finalizing;
  m_impl->wake.notify_one();
}
void FfmpegVideoEncoder::cancel()
{
  std::lock_guard lock(m_impl->mutex);
  if (active(m_impl->status.state)) {
    m_impl->cancelled = true;
    m_impl->queue.clear();
    m_impl->wake.notify_one();
  }
}
