#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <cstdint>
#include <filesystem>
#include <imgui/imgui.h>
#include <memory>
#include <stdexcept>
#include <utility>
#include <vector>

#include "app/VideoExportController.hpp"
#include "app/VisualizerController.hpp"
#include "core/CoreTypes.hpp"
#include "export/ExportSnapshot.hpp"
#include "export/VideoEncoder.hpp"
#include "export/VideoExportSettings.hpp"
#include "midi/MidiTimeline.hpp"
#include "ui/VideoExportControls.hpp"

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
    current = {.state = VideoEncoderState::Running, .framesWritten = 0, .error = {}};
    cancelRequested = false;
  }
  [[nodiscard]] VideoEncoderStatus status() const override
  {
    return current;
  }
  [[nodiscard]] bool canAcceptFrame() const override
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
struct SongOptions
{
  double length = 0.1;
  double offset = -0.2;
  double rate = 2.0;
};

ExportSnapshot song(SongOptions options = {})
{
  const auto [length, offset, rate] = options;
  MidiTimeline timeline;
  timeline.addNote({.pitch = 60, .velocity = 64, .durationSeconds = length});
  return {std::move(timeline), {}, {.r = 0.2f, .g = 0.3f, .b = 0.4f, .a = 1.0f}, offset, rate};
}
} // namespace

namespace {

TEST_CASE("Video controller freezes all 54 visual export profiles", "[export-controller]")
{
  for (const auto fps : {30, 60}) {
    for (const auto quality : {ExportQuality::Medium, ExportQuality::High, ExportQuality::Best}) {
      for (const auto resolution :
           {ExportResolution::P720, ExportResolution::P1080, ExportResolution::P2160}) {
        for (const auto aspect : {ExportAspectRatio::Landscape,
                                  ExportAspectRatio::Portrait,
                                  ExportAspectRatio::Square}) {
          auto encoder = std::make_unique<FakeVideoEncoder>();
          auto* fake = encoder.get();
          VideoExportSettings settings{
            .fps = fps, .quality = quality, .resolution = resolution, .aspectRatio = aspect};
          const int shortSide = resolution == ExportResolution::P720    ? 720
                                : resolution == ExportResolution::P1080 ? 1080
                                                                        : 2160;
          const int width = aspect == ExportAspectRatio::Landscape ? shortSide * 16 / 9 : shortSide;
          const int height = aspect == ExportAspectRatio::Portrait ? shortSide * 16 / 9 : shortSide;
          VideoExportController controller(std::move(encoder),
                                           [&](const auto&, double, FramebufferSize size) {
                                             CHECK(size.width == width);
                                             CHECK(size.height == height);
                                             return std::vector<std::uint8_t>{};
                                           });
          REQUIRE(controller.start(song({.length = 1, .offset = 0, .rate = 1}),
                                   "profile.mp4",
                                   VideoOverwritePolicy::RefuseExisting,
                                   settings));
          settings.fps = 1; // Changes to the caller's settings cannot affect the active job.
          CHECK(controller.frameCount() == static_cast<std::uint64_t>(3 * fps));
          CHECK(fake->config.fps == fps);
          CHECK(fake->config.quality == quality);
          CHECK(fake->config.size.width == width);
          CHECK(fake->config.size.height == height);
          controller.tick();
          CHECK(controller.framesGenerated() == 1);
        }
      }
    }
  }
}

TEST_CASE("Video controller forwards explicit overwrite consent through both start overloads",
          "[export-controller]")
{
  auto encoder = std::make_unique<FakeVideoEncoder>();
  auto* fake = encoder.get();
  VideoExportController controller(std::move(encoder), [](const auto&, double, FramebufferSize) {
    return std::vector<std::uint8_t>{};
  });
  REQUIRE(
    controller.start([] { return song(); }, "existing.mp4", VideoOverwritePolicy::ReplaceExisting));
  CHECK(fake->config.overwritePolicy == VideoOverwritePolicy::ReplaceExisting);
  controller.cancel();
  REQUIRE(controller.start(song(), "existing.mp4", VideoOverwritePolicy::ReplaceExisting));
  CHECK(fake->config.overwritePolicy == VideoOverwritePolicy::ReplaceExisting);
}

TEST_CASE("Export controls display generated frames only while exporting",
          "[export-controller][ui]")
{
  ImGui::CreateContext();
  struct ContextGuard
  {
    ~ContextGuard()
    {
      ImGui::DestroyContext();
    }
  } const guard;
  auto& io = ImGui::GetIO();
  io.IniFilename = nullptr;
  io.DisplaySize = {1000, 1000};
  REQUIRE(io.Fonts->Build());
  VideoExportController controller(std::make_unique<FakeVideoEncoder>(),
                                   [](const auto&, double, FramebufferSize) {
                                     return std::vector<std::uint8_t>{};
                                   });
  for (const auto aspect :
       {ExportAspectRatio::Landscape, ExportAspectRatio::Portrait, ExportAspectRatio::Square}) {
    VideoExportSettings settings{.resolution = ExportResolution::P2160, .aspectRatio = aspect};
    int idleCalls = 0;
    ImGui::NewFrame();
    VideoExportControls::render(controller, true, settings, [&](FramebufferSize) {
      ++idleCalls;
      return std::uint64_t{0};
    });
    ImGui::EndFrame();
    CHECK(idleCalls == 0);
    REQUIRE(
      controller.start(song(), "preview.mp4", VideoOverwritePolicy::RefuseExisting, settings));
    controller.tick();
    REQUIRE(controller.framesGenerated() == 1);
    int calls = 0;
    for (int frame = 0; frame < 2; ++frame) {
      ImGui::NewFrame();
      VideoExportControls::render(controller, true, settings, [&](FramebufferSize size) {
        ++calls;
        CHECK(size.width == (aspect == ExportAspectRatio::Landscape ? 3840 : 2160));
        CHECK(size.height == (aspect == ExportAspectRatio::Portrait ? 3840 : 2160));
        return std::uint64_t{0};
      });
      ImGui::EndFrame();
    }
    CHECK(calls > 0);
    controller.cancel();
  }
}

TEST_CASE("Video controller uses frozen full song timing with a selected 30 FPS profile",
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
  REQUIRE(controller.start(song(), "test.mp4", VideoOverwritePolicy::RefuseExisting, {.fps = 30}));
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
  CHECK(fake->config.overwritePolicy == VideoOverwritePolicy::RefuseExisting);
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
  REQUIRE(controller.start(song({.length = 100000}), "test.mp4"));
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
  fake->current = {.state = VideoEncoderState::Failed,
                   .framesWritten = 0,
                   .error = "encoder crashed"};
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
} // namespace
