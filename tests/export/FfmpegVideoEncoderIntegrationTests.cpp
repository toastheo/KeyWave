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

TEST_CASE("Encoder detects missing codec and child crashes without publishing", "[encoder]")
{
  TempDirectory temp;
  for (const auto name : {"no-codec.exe", "crash.exe"}) {
    const auto executable = temp.path / name;
    std::filesystem::copy_file(KEYWAVE_PROCESS_FIXTURE, executable);
    FfmpegVideoEncoder encoder(executable);
    encoder.start({{256, 256}, 30, 3}, temp.path / "output.mp4");
    auto status = waitFor(encoder, true);
    if (status.state == VideoEncoderState::Running) {
      for (int i = 0; i < 3; ++i) { encoder.trySubmit(std::vector<std::uint8_t>(256 * 256 * 4)); }
      encoder.finish();
      status = waitFor(encoder);
    }
    CHECK(status.state == VideoEncoderState::Failed);
    CHECK_FALSE(status.error.empty());
    CHECK(status.error.size() < 66000);
    CHECK_FALSE(std::filesystem::exists(temp.path / "output.mp4"));
    CHECK(std::distance(std::filesystem::directory_iterator(temp.path),
                        std::filesystem::directory_iterator{}) == 1);
    std::filesystem::remove(executable);
  }
}

TEST_CASE("Blocked consumer bounds frame queue and cancels promptly", "[encoder]")
{
  TempDirectory temp;
  const auto executable = temp.path / "block.exe";
  std::filesystem::copy_file(KEYWAVE_PROCESS_FIXTURE, executable);
  FfmpegVideoEncoder encoder(executable);
  encoder.start({{256, 256}, 30, 1000000}, temp.path / "output.mp4");
  REQUIRE(waitFor(encoder, true).state == VideoEncoderState::Running);
  CHECK_THROWS(encoder.finish());
  CHECK_THROWS(encoder.trySubmit(std::vector<std::uint8_t>(1)));
  int accepted = 0;
  while (encoder.trySubmit(std::vector<std::uint8_t>(256 * 256 * 4))) { ++accepted; }
  CHECK(accepted <= static_cast<int>(FfmpegVideoEncoder::queueCapacity + 1));
  std::vector<std::uint8_t> rejected(256 * 256 * 4);
  CHECK_FALSE(encoder.trySubmit(std::move(rejected)));
  CHECK_FALSE(rejected.empty());
  const auto before = std::chrono::steady_clock::now();
  encoder.cancel();
  CHECK(waitFor(encoder).state == VideoEncoderState::Cancelled);
  CHECK(std::chrono::steady_clock::now() - before < std::chrono::seconds(2));
  CHECK(std::distance(std::filesystem::directory_iterator(temp.path),
                      std::filesystem::directory_iterator{}) == 1);
}

TEST_CASE("Successful encoder publishes only on completion and protects destination races", "[encoder]")
{
  TempDirectory temp;
  const auto executable = temp.path / "success.exe";
  std::filesystem::copy_file(KEYWAVE_PROCESS_FIXTURE, executable);
  FfmpegVideoEncoder encoder(executable);
  const auto output = temp.path / "output.mp4";
  for (int attempt = 0; attempt < 2; ++attempt) {
    encoder.start({{16, 16}, 30, 1}, output);
    REQUIRE(waitFor(encoder, true).state == VideoEncoderState::Running);
    CHECK_FALSE(std::filesystem::exists(output));
    REQUIRE(encoder.trySubmit(std::vector<std::uint8_t>(16 * 16 * 4)));
    CHECK_FALSE(encoder.trySubmit(std::vector<std::uint8_t>(16 * 16 * 4)));
    if (attempt == 1) { std::ofstream file(output); file << "keep me"; }
    encoder.finish();
    const auto status = waitFor(encoder);
    CHECK(status.state == (attempt == 0 ? VideoEncoderState::Completed : VideoEncoderState::Failed));
    CHECK(std::filesystem::file_size(output) == (attempt == 0 ? 5 : 7));
    CHECK(std::distance(std::filesystem::directory_iterator(temp.path),
                        std::filesystem::directory_iterator{}) == 2);
    std::filesystem::remove(output);
  }
}