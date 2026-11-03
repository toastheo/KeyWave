#include <array>
#include <atomic>
#include <catch2/catch_test_macros.hpp>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <ios>
#include <iterator>
#include <memory>
#include <string>
#include <system_error>
#include <thread>
#include <utility>
#include <vector>

#include "catch2/catch_message.hpp"
#include "core/CoreTypes.hpp"
#include "export/ExportComposition.hpp"
#include "export/ExportSnapshot.hpp"
#include "export/VideoEncoder.hpp"
#include "export/VideoExportSettings.hpp"
#include "export_ffmpeg/FfmpegVideoEncoder.hpp"
#include "midi/MidiTimeline.hpp"
#include "platform/ChildProcess.hpp"
#include "render_opengl/OffscreenRenderTarget.hpp"

#if defined(KEYWAVE_TEST_GPU_EXPORT)
#include <glad/gl.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include "app/VideoExportController.hpp"
#include "render_opengl/OpenGLRendererBackend.hpp"
#endif

namespace {
struct Directory
{
  std::filesystem::path path =
    std::filesystem::temp_directory_path() /
    ("keywave-real-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
  Directory()
  {
    std::filesystem::create_directory(path);
  }
  ~Directory()
  {
    std::error_code error;
    std::filesystem::remove_all(path, error);
  }
};
std::string utf8(const std::filesystem::path& path)
{
  const auto text = path.u8string();
  return {reinterpret_cast<const char*>(text.data()), text.size()};
}
void assertVideo(const std::filesystem::path& file,
                 int frames,
                 const std::string& duration,
                 FramebufferSize size = {.width = 1920, .height = 1080},
                 int fps = 30)
{
  std::atomic_bool const cancelled{false};
  ChildProcess decode(ChildProcess::findExecutable("ffmpeg"),
                      {"-hide_banner",
                       "-nostdin",
                       "-i",
                       utf8(file),
                       "-map",
                       "0",
                       "-progress",
                       "pipe:1",
                       "-f",
                       "null",
                       "-"});
  decode.closeInput();
  REQUIRE(decode.wait(cancelled) == 0); // Fully decode all streams, detecting corrupt MP4s.
  const auto output = decode.diagnostics();
  INFO(output);
  CHECK(output.find("Video: h264") != std::string::npos);
  CHECK(output.find(std::to_string(size.width) + "x" + std::to_string(size.height)) !=
        std::string::npos);
  CHECK(output.find(std::to_string(fps) + " fps") != std::string::npos);
  CHECK(output.find("tv, bt709") != std::string::npos);
  CHECK(output.find("Audio:") == std::string::npos);
  CHECK(output.find("Duration: " + duration) != std::string::npos);
  CHECK((output.find("frame=" + std::to_string(frames) + "\n") != std::string::npos ||
         output.find("frame=" + std::to_string(frames) + "\r\n") != std::string::npos));
  CHECK(output.find("progress=end") != std::string::npos);
}
} // namespace

namespace {

TEST_CASE("Real FFmpeg preserves known colors using limited-range BT709 YUV", "[ffmpeg]")
{
  Directory const temp;
  const auto output = temp.path / "colors.mp4";
  constexpr std::array<std::array<int, 3>, 6> rgb{
    {{0, 0, 0}, {255, 255, 255}, {255, 0, 0}, {0, 255, 0}, {0, 0, 255}, {128, 128, 128}}};
  // Independent rounded 8-bit BT.709 limited-range Y, Cb, Cr values.
  constexpr std::array<std::array<int, 3>, 6> yuv{{{16, 128, 128},
                                                   {235, 128, 128},
                                                   {63, 102, 240},
                                                   {173, 42, 26},
                                                   {32, 240, 118},
                                                   {126, 128, 128}}};
  FfmpegVideoEncoder encoder;
  encoder.start({.size = {.width = 128, .height = 128},
                 .fps = 60,
                 .frameCount = 6,
                 .quality = ExportQuality::Best},
                output);
  std::size_t submitted = 0;
  const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(30);
  while (std::chrono::steady_clock::now() < deadline &&
         encoder.status().state != VideoEncoderState::Completed) {
    INFO(encoder.status().error);
    REQUIRE(encoder.status().state != VideoEncoderState::Failed);
    if (submitted < rgb.size() && encoder.canAcceptFrame()) {
      std::vector<std::uint8_t> pixels(std::size_t{128} * 128 * 4);
      for (std::size_t p = 0; p < pixels.size(); p += 4) {
        for (std::size_t c = 0; c < 3; ++c) {
          pixels[p + c] = static_cast<std::uint8_t>(rgb[submitted][c]);
        }
        pixels[p + 3] = 255;
      }
      REQUIRE(encoder.trySubmit(std::move(pixels)));
      if (++submitted == rgb.size()) {
        encoder.finish();
      }
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
  REQUIRE(encoder.status().state == VideoEncoderState::Completed);
  assertVideo(output, 6, "00:00:00.10", {.width = 128, .height = 128}, 60);
  const auto raw = temp.path / "decoded.yuv";
  ChildProcess decode(
    ChildProcess::findExecutable("ffmpeg"),
    {"-v", "error", "-i", utf8(output), "-pix_fmt", "yuv420p", "-f", "rawvideo", utf8(raw)});
  decode.closeInput();
  const std::atomic_bool cancelled{false};
  REQUIRE(decode.wait(cancelled) == 0);
  std::ifstream input(raw, std::ios::binary);
  const std::vector<unsigned char> bytes{std::istreambuf_iterator<char>(input), {}};
  constexpr std::size_t lumaBytes = std::size_t{128} * 128;
  constexpr std::size_t chromaBytes = std::size_t{64} * 64;
  constexpr std::size_t frameBytes = lumaBytes + 2 * chromaBytes;
  REQUIRE(bytes.size() == 6 * frameBytes);
  for (std::size_t i = 0; i < yuv.size(); ++i) {
    CHECK(std::abs(int(bytes[i * frameBytes + std::size_t{64} * 128 + 64]) - yuv[i][0]) <= 3);
    CHECK(std::abs(int(bytes[i * frameBytes + lumaBytes + std::size_t{32} * 64 + 32]) -
                   yuv[i][1]) <= 3);
    CHECK(
      std::abs(int(bytes[i * frameBytes + lumaBytes + chromaBytes + std::size_t{32} * 64 + 32]) -
               yuv[i][2]) <= 3);
  }
}

TEST_CASE("Real FFmpeg streams a silent Unicode-path 1080p MP4 with exact frame count", "[ffmpeg]")
{
  Directory const temp;
  const auto output = temp.path / std::filesystem::path(u8"Klavier ä 日本 & export.mp4");
  {
    std::ofstream previous(output);
    previous << "old video";
  }
  FfmpegVideoEncoder encoder;
  encoder.start({.size = {.width = 1920, .height = 1080},
                 .fps = 30,
                 .frameCount = 3,
                 .overwritePolicy = VideoOverwritePolicy::ReplaceExisting},
                output);
  int submitted = 0;
  const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(30);
  while (std::chrono::steady_clock::now() < deadline) {
    auto status = encoder.status();
    INFO(status.error);
    REQUIRE(status.state != VideoEncoderState::Failed);
    if (status.state == VideoEncoderState::Completed) {
      break;
    }
    if (encoder.canAcceptFrame()) {
      if (submitted == 0) {
        CHECK(std::filesystem::file_size(output) == 9);
      }
      std::vector<std::uint8_t> frame(std::size_t{1920} * 1080 * 4);
      for (std::size_t p = 0; p < frame.size(); p += 4) {
        frame[p] = static_cast<std::uint8_t>(submitted * 80);
        frame[p + 3] = 255;
      }
      REQUIRE(encoder.trySubmit(std::move(frame)));
      if (++submitted == 3) {
        encoder.finish();
      }
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(2));
  }
  REQUIRE(encoder.status().state == VideoEncoderState::Completed);
  assertVideo(output, 3, "00:00:00.10");
  std::atomic_bool const cancelled{false};
  ChildProcess colors(ChildProcess::findExecutable("ffmpeg"),
                      {"-v",
                       "error",
                       "-i",
                       utf8(output),
                       "-vf",
                       "format=rgba,scale=1:1",
                       "-pix_fmt",
                       "rgba",
                       "-f",
                       "rawvideo",
                       "pipe:1"});
  colors.closeInput();
  REQUIRE(colors.wait(cancelled) == 0);
  const auto pixels = colors.diagnostics();
  REQUIRE(pixels.size() == 12);
  for (std::size_t i = 0; i < 3; ++i) {
    CHECK(std::abs(static_cast<int>(static_cast<unsigned char>(pixels[i * 4])) -
                   static_cast<int>(i * 80)) <= 4);
  }
}

#if defined(KEYWAVE_TEST_GPU_EXPORT)
TEST_CASE("Visual profiles export real GPU frames at 720p30 portrait1080p60 and 4K60",
          "[ffmpeg][gpu]")
{
  REQUIRE(glfwInit() == GLFW_TRUE);
  glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  auto* window = glfwCreateWindow(160, 90, "KeyWave M4 acceptance", nullptr, nullptr);
  REQUIRE(window != nullptr);
  struct ContextGuard
  {
    GLFWwindow* window;
    ~ContextGuard()
    {
      glfwDestroyWindow(window);
      glfwTerminate();
    }
  } const guard{window};
  glfwMakeContextCurrent(window);
  REQUIRE(gladLoadGL(glfwGetProcAddress) != 0);
  OpenGLRendererBackend renderer(glfwGetProcAddress, {.r = 0, .g = 0, .b = 0, .a = 1});
  REQUIRE(renderer.initialize());
  Directory const temp;
  const std::array profiles{VideoExportSettings{.fps = 30,
                                                .quality = ExportQuality::Medium,
                                                .resolution = ExportResolution::P720},
                            VideoExportSettings{.fps = 60,
                                                .quality = ExportQuality::High,
                                                .resolution = ExportResolution::P1080,
                                                .aspectRatio = ExportAspectRatio::Portrait},
                            VideoExportSettings{.fps = 60,
                                                .quality = ExportQuality::Best,
                                                .resolution = ExportResolution::P2160}};
  for (auto settings : profiles) {
    const auto size = exportFramebufferSize(settings);
    CAPTURE(size.width, size.height, settings.fps);
    OffscreenRenderTarget target(size);
    VideoExportController controller(
      std::make_unique<FfmpegVideoEncoder>(),
      [&](const ExportSnapshot& snapshot, double time, FramebufferSize frameSize) {
        const auto scene = buildExportScene(
          snapshot.timeline, time, snapshot.sceneConfig, frameSize, snapshot.compositionOptions);
        auto pixels = renderer.renderOffscreen(target, scene, snapshot.background).pixels;
        // The live 16:9 view has 216px-tall white keys at 1920px width.
        // Changing output aspect must retain that displayed height-to-width ratio.
        const auto x = static_cast<std::size_t>(frameSize.width / 104);
        const int keyHeight = frameSize.width * 9 / 80;
        const auto whiteOffset = (static_cast<std::size_t>(frameSize.height - keyHeight / 2) *
                                    static_cast<std::size_t>(frameSize.width) +
                                  x) *
                                 4;
        const auto aboveOffset = (static_cast<std::size_t>(frameSize.height - keyHeight - 10) *
                                    static_cast<std::size_t>(frameSize.width) +
                                  x) *
                                 4;
        CHECK(pixels[whiteOffset] == 235);
        CHECK(pixels[whiteOffset + 1] == 235);
        CHECK(std::abs(int(pixels[whiteOffset + 2]) - 230) <= 1);
        CHECK(pixels[aboveOffset] <= 27);
        return pixels;
      });
    MidiTimeline timeline;
    timeline.addNote({.pitch = 60, .velocity = 80, .durationSeconds = 0.2});
    settings.range = {.mode = ExportRangeMode::Clip, .startSeconds = 0, .endSeconds = 0.1};
    const auto output = temp.path / (std::to_string(size.width) + ".mp4");
    REQUIRE(controller.start(ExportSnapshot{timeline, {}, {0.1f, 0.2f, 0.3f, 1}, 0, 1},
                             output,
                             VideoOverwritePolicy::RefuseExisting,
                             settings));
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(60);
    while (controller.active() && std::chrono::steady_clock::now() < deadline) {
      glfwPollEvents();
      controller.tick();
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    INFO(controller.status().error);
    REQUIRE(controller.status().state == VideoEncoderState::Completed);
    assertVideo(output, settings.fps / 10, "00:00:00.10", size, settings.fps);
    CHECK(glGetError() == GL_NO_ERROR);
  }
}

TEST_CASE("Full test song exports from snapshot through GPU and FFmpeg", "[ffmpeg][gpu]")
{
  REQUIRE(glfwInit() == GLFW_TRUE);
  glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  auto* window = glfwCreateWindow(160, 90, "KeyWave export acceptance", nullptr, nullptr);
  REQUIRE(window != nullptr);
  struct ContextGuard
  {
    GLFWwindow* window;
    ~ContextGuard()
    {
      glfwDestroyWindow(window);
      glfwTerminate();
    }
  } const guard{window};
  glfwMakeContextCurrent(window);
  REQUIRE(gladLoadGL(glfwGetProcAddress) != 0);
  OpenGLRendererBackend renderer(glfwGetProcAddress, {.r = 0, .g = 0, .b = 0, .a = 1});
  renderer.setFramebufferSize({.width = 160, .height = 90});
  REQUIRE(renderer.initialize());
  OffscreenRenderTarget target({.width = 1920, .height = 1080});
  VideoExportController controller(
    std::make_unique<FfmpegVideoEncoder>(),
    [&](const ExportSnapshot& snapshot, double time, FramebufferSize) {
      const auto scene = buildExportScene(
        snapshot.timeline, time, snapshot.sceneConfig, target.size(), snapshot.compositionOptions);
      return renderer.renderOffscreen(target, scene, snapshot.background).pixels;
    });
  MidiTimeline timeline;
  timeline.addNote({.pitch = 60, .velocity = 80, .startSeconds = 0.1, .durationSeconds = 0.4});
  Directory const temp;
  const auto output = temp.path / "full-song.mp4";
  REQUIRE(controller.start(ExportSnapshot{timeline, {}, {0.1f, 0.2f, 0.3f, 1}, -0.5, 1},
                           output,
                           VideoOverwritePolicy::RefuseExisting,
                           {.fps = 30}));
  CHECK(controller.frameCount() == 90); // 1s playback including lead-in + 2s ending.
  const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(45);
  while (controller.active() && std::chrono::steady_clock::now() < deadline) {
    glfwPollEvents();
    controller.tick();
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
  INFO(controller.status().error);
  REQUIRE(controller.status().state == VideoEncoderState::Completed);
  CHECK(controller.framesGenerated() == 90);
  assertVideo(output, 90, "00:00:03.00");
  CHECK(glGetError() == GL_NO_ERROR);
}
#endif

} // namespace
