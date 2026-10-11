#include <catch2/catch_test_macros.hpp>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <thread>

#include "export_ffmpeg/FfmpegVideoEncoder.hpp"

namespace {
bool terminal(VideoEncoderState state)
{
  return state == VideoEncoderState::Completed || state == VideoEncoderState::Failed ||
         state == VideoEncoderState::Cancelled;
}
VideoEncoderStatus waitFor(FfmpegVideoEncoder& encoder, bool ready = false)
{
  const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(15);
  while (std::chrono::steady_clock::now() < deadline) {
    auto status = encoder.status();
    if (terminal(status.state) || (ready && status.state == VideoEncoderState::Running)) {
      return status;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(2));
  }
  FAIL("encoder did not respond within deadline");
  return {};
}
struct TempDirectory
{
  std::filesystem::path path = std::filesystem::temp_directory_path() /
    ("keywave-m3-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
  TempDirectory() { std::filesystem::create_directory(path); }
  ~TempDirectory() { std::error_code error; std::filesystem::remove_all(path, error); }
};
}

TEST_CASE("Missing FFmpeg fails asynchronously without publishing a video", "[encoder]")
{
  TempDirectory temp;
  FfmpegVideoEncoder encoder(temp.path / "missing-ffmpeg.exe");
  encoder.start({{16, 16}, 30, 1}, temp.path / "output.mp4");
  const auto status = waitFor(encoder);
  CHECK(status.state == VideoEncoderState::Failed);
  CHECK_FALSE(status.error.empty());
  CHECK(std::filesystem::is_empty(temp.path));
}

TEST_CASE("Encoder refuses invalid profiles and existing destinations", "[encoder]")
{
  TempDirectory temp;
  const auto output = temp.path / "existing.mp4";
  { std::ofstream file(output); file << "keep me"; }
  FfmpegVideoEncoder encoder;
  CHECK_THROWS(encoder.start({{16, 16}, 30, 1}, output));
  CHECK_THROWS(encoder.start({{0, 16}, 30, 1}, temp.path / "out.mp4"));
  CHECK_THROWS(encoder.start({{17, 16}, 30, 1}, temp.path / "out.mp4"));
  CHECK_THROWS(encoder.start({{16, 16}, 0, 1}, temp.path / "out.mp4"));
  CHECK_THROWS(encoder.start({{16, 16}, 30, 0}, temp.path / "out.mp4"));
  CHECK(std::filesystem::file_size(output) == 7);
}
