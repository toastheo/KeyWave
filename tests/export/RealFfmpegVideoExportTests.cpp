#include <catch2/catch_test_macros.hpp>
#include <atomic>
#include <chrono>
#include <filesystem>
#include <thread>

#include "export_ffmpeg/FfmpegVideoEncoder.hpp"
#include "platform/ChildProcess.hpp"

#if defined(KEYWAVE_TEST_GPU_EXPORT)
#include <glad/gl.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include "app/VideoExportController.hpp"
#include "fallingnotes/PianoRollSceneBuilder.hpp"
#include "render_opengl/OpenGLRendererBackend.hpp"
#endif

namespace {
struct Directory
{
  std::filesystem::path path = std::filesystem::temp_directory_path() /
    ("keywave-real-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
  Directory() { std::filesystem::create_directory(path); }
  ~Directory() { std::error_code error; std::filesystem::remove_all(path, error); }
};
std::string utf8(const std::filesystem::path& path)
{
  const auto text = path.u8string();
  return {reinterpret_cast<const char*>(text.data()), text.size()};
}
void assertVideo(const std::filesystem::path& file, int frames, const std::string& duration)
{
  std::atomic_bool cancelled{false};
  ChildProcess decode(ChildProcess::findExecutable("ffmpeg"), {"-hide_banner", "-nostdin",
    "-i", utf8(file), "-map", "0", "-progress", "pipe:1", "-f", "null", "-"});
  decode.closeInput();
  REQUIRE(decode.wait(cancelled) == 0); // Fully decode all streams, detecting corrupt MP4s.
  const auto output = decode.diagnostics();
  INFO(output);
  CHECK(output.find("Video: h264") != std::string::npos);
  CHECK(output.find("1920x1080") != std::string::npos);
  CHECK(output.find("30 fps") != std::string::npos);
  CHECK(output.find("Audio:") == std::string::npos);
  CHECK(output.find("Duration: " + duration) != std::string::npos);
  CHECK((output.find("frame=" + std::to_string(frames) + "\n") != std::string::npos ||
         output.find("frame=" + std::to_string(frames) + "\r\n") != std::string::npos));
  CHECK(output.find("progress=end") != std::string::npos);
}
}

TEST_CASE("Real FFmpeg streams a silent Unicode-path 1080p MP4 with exact frame count", "[ffmpeg]")
{
  Directory temp;
  const auto output = temp.path / std::filesystem::path(u8"Klavier ä 日本 & export.mp4");
  FfmpegVideoEncoder encoder;
  encoder.start({{1920, 1080}, 30, 3}, output);
  int submitted = 0;
  const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(30);
  while (std::chrono::steady_clock::now() < deadline) {
    auto status = encoder.status();
    INFO(status.error);
    REQUIRE(status.state != VideoEncoderState::Failed);
    if (status.state == VideoEncoderState::Completed) { break; }
    if (encoder.canAcceptFrame()) {
      std::vector<std::uint8_t> frame(1920 * 1080 * 4);
      for (std::size_t p = 0; p < frame.size(); p += 4) {
        frame[p] = static_cast<std::uint8_t>(submitted * 80);
        frame[p + 3] = 255;
      }
      REQUIRE(encoder.trySubmit(std::move(frame)));
      if (++submitted == 3) { encoder.finish(); }
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(2));
  }
  REQUIRE(encoder.status().state == VideoEncoderState::Completed);
  assertVideo(output, 3, "00:00:00.10");
}

#if defined(KEYWAVE_TEST_GPU_EXPORT)
TEST_CASE("Full test song exports from snapshot through GPU and FFmpeg", "[ffmpeg][gpu]")
{
  REQUIRE(glfwInit() == GLFW_TRUE);
  glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  auto* window = glfwCreateWindow(160, 90, "KeyWave export acceptance", nullptr, nullptr);
  REQUIRE(window != nullptr);
  struct ContextGuard { GLFWwindow* window; ~ContextGuard() {
    glfwDestroyWindow(window); glfwTerminate(); } } guard{window};
  glfwMakeContextCurrent(window);
  REQUIRE(gladLoadGL(glfwGetProcAddress) != 0);
  OpenGLRendererBackend renderer(glfwGetProcAddress, {0, 0, 0, 1});
  renderer.setFramebufferSize({160, 90});
  REQUIRE(renderer.initialize());
  OffscreenRenderTarget target({1920, 1080});
  VideoExportController controller(std::make_unique<FfmpegVideoEncoder>(),
    [&](const ExportSnapshot& snapshot, double time, FramebufferSize) {
      const auto scene = PianoRollSceneBuilder::build(snapshot.timeline, time, snapshot.sceneConfig);
      return renderer.renderOffscreen(target, scene, snapshot.background).pixels;
    });
  MidiTimeline timeline;
  timeline.addNote({.pitch = 60, .velocity = 80, .startSeconds = 0.1, .durationSeconds = 0.4});
  Directory temp;
  const auto output = temp.path / "full-song.mp4";
  REQUIRE(controller.start(ExportSnapshot{timeline, {}, {0.1f, 0.2f, 0.3f, 1}, -0.5, 1}, output));
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
