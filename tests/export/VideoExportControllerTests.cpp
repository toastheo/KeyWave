#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <stdexcept>

#include "app/VideoExportController.hpp"
#include "app/VisualizerController.hpp"

namespace {
class FakeVideoEncoder final : public VideoEncoder
{
public:
  VideoEncoderStatus current;
  VideoEncoderConfig config;
  bool blocked = false;
  bool cancelRequested = false;
  void start(VideoEncoderConfig value, const std::filesystem::path&) override
  {
    config = value;
    current = {VideoEncoderState::Running, 0, {}};
    cancelRequested = false;
  }
  VideoEncoderStatus status() const override
  {
    return current;
  }
  bool canAcceptFrame() const override
  {
    return !blocked;
  }
  bool trySubmit(std::vector<std::uint8_t>&&) override
  {
    ++current.framesWritten;
    return true;
  }
  void finish() override
  {
    current.state = VideoEncoderState::Finalizing;
  }
  void cancel() override
  {
    cancelRequested = true;
    current.state = VideoEncoderState::Cancelled;
  }
};
ExportSnapshot song(double length = 0.1, double offset = -0.2, double rate = 2.0)
{
  MidiTimeline timeline;
  timeline.addNote({.pitch = 60, .velocity = 64, .durationSeconds = length});
  return {std::move(timeline), {}, {0.2f, 0.3f, 0.4f, 1.0f}, offset, rate};
}
} // namespace

TEST_CASE("Video controller uses frozen full song timing and fixed MVP profile",
          "[export-controller]")
{
  auto encoder = std::make_unique<FakeVideoEncoder>();
  auto* fake = encoder.get();
  std::vector<double> times;
  VideoExportController controller(
    std::move(encoder), [&](const ExportSnapshot& snapshot, double time, FramebufferSize size) {
      CHECK(snapshot.background.r == Catch::Approx(0.2f));
      CHECK(size.width == 1920);
      CHECK(size.height == 1080);
      times.push_back(time);
      return std::vector<std::uint8_t>{};
    });
  REQUIRE(controller.start(song(), "test.mp4"));
  CHECK(controller.active());
  CHECK(controller.frameCount() == 65); // (0.1 - -0.2) / 2 + 2s tail, ceil at 30 fps
  CHECK(controller.outputDurationSeconds() == Catch::Approx(65.0 / 30));
  fake->blocked = true;
  for (int i = 0; i < 100; ++i) {
    controller.tick();
  }
  CHECK(times.empty());
  CHECK(controller.framesGenerated() == 0);
  fake->blocked = false;
  controller.tick();
  REQUIRE(times.size() == 1);
  CHECK(times[0] == Catch::Approx(-0.2));
  controller.tick();
  CHECK(times[1] == Catch::Approx(-0.2 + 2.0 / 30));
  CHECK_THROWS(controller.start(song(), "second.mp4"));
  for (int i = 2; i < 65; ++i) {
    controller.tick();
  }
  CHECK(controller.framesGenerated() == 65);
  CHECK(controller.status().state == VideoEncoderState::Finalizing);
  CHECK(times.back() == Catch::Approx(0.1));
  fake->current.state = VideoEncoderState::Completed;
  controller.tick();
  CHECK_FALSE(controller.active());
  CHECK(controller.status().state == VideoEncoderState::Completed);
  CHECK(fake->config.fps == 30);
  CHECK(fake->config.frameCount == 65);
}

TEST_CASE("Video controller cancels a blocked export and permits another job",
          "[export-controller]")
{
  auto encoder = std::make_unique<FakeVideoEncoder>();
  auto* fake = encoder.get();
  int renders = 0;
  VideoExportController controller(std::move(encoder), [&](const auto&, double, FramebufferSize) {
    ++renders;
    return std::vector<std::uint8_t>{};
  });
  REQUIRE(controller.start(song(100000), "test.mp4"));
  fake->blocked = true;
  controller.tick();
  controller.cancel();
  controller.tick();
  CHECK(controller.status().state == VideoEncoderState::Cancelled);
  CHECK(renders == 0);
  REQUIRE(controller.start(song(), "next.mp4"));
  fake->blocked = false;
  controller.tick();
  CHECK(controller.framesGenerated() == 1);
}

TEST_CASE("Video controller reports GPU failure and stops the encoder", "[export-controller]")
{
  auto encoder = std::make_unique<FakeVideoEncoder>();
  auto* fake = encoder.get();
  VideoExportController controller(
    std::move(encoder), [](const auto&, double, FramebufferSize) -> std::vector<std::uint8_t> {
      throw std::runtime_error("GPU readback failed");
    });
  REQUIRE(controller.start(song(), "test.mp4"));
  controller.tick();
  CHECK(controller.status().state == VideoEncoderState::Failed);
  CHECK(controller.status().error == "GPU readback failed");
  CHECK(fake->cancelRequested);
  CHECK(controller.framesGenerated() == 0);
}

TEST_CASE("Video controller propagates asynchronous encoder errors without more rendering",
          "[export-controller]")
{
  auto encoder = std::make_unique<FakeVideoEncoder>();
  auto* fake = encoder.get();
  int renders = 0;
  VideoExportController controller(std::move(encoder), [&](const auto&, double, FramebufferSize) {
    ++renders;
    return std::vector<std::uint8_t>{};
  });
  REQUIRE(controller.start(song(), "test.mp4"));
  fake->current = {VideoEncoderState::Failed, 0, "encoder crashed"};
  controller.tick();
  CHECK(controller.status().error == "encoder crashed");
  CHECK_FALSE(controller.active());
  CHECK(renders == 0);
}

TEST_CASE("Exporting an empty loaded MIDI reports failure and allows retry", "[export-controller]")
{
  VisualizerController visualizer;
  visualizer.setTimeline(MidiTimeline{});
  REQUIRE(visualizer.hasTimeline());
  auto encoder = std::make_unique<FakeVideoEncoder>();
  int renders = 0;
  VideoExportController controller(std::move(encoder), [&](const auto&, double, FramebufferSize) {
    ++renders;
    return std::vector<std::uint8_t>{};
  });
  CHECK_FALSE(controller.start([&] { return visualizer.exportSnapshot(); }, "empty.mp4"));
  CHECK(controller.status().state == VideoEncoderState::Failed);
  CHECK_FALSE(controller.status().error.empty());
  CHECK_FALSE(controller.active());
  CHECK(renders == 0);
  REQUIRE(controller.start(song(), "valid.mp4"));
  controller.tick();
  CHECK(controller.framesGenerated() == 1);
}