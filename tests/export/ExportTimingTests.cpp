#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <type_traits>

#include "app/VisualizerController.hpp"
#include "export/ExportTiming.hpp"
#include "midi/MidiFileLoader.hpp"
#include "midi/MidiFixtures.hpp"

namespace {
ExportSnapshot snapshot(double length = 10.0, double offset = 0.0, double rate = 1.0)
{
  MidiTimeline timeline;
  timeline.addNote({.pitch = 60, .velocity = 64, .startSeconds = 0.0, .durationSeconds = length});
  return ExportSnapshot{timeline, {}, {}, offset, rate};
}

VideoExportSettings clip(double start = 0.0, double end = 10.0)
{
  VideoExportSettings settings;
  settings.range = {ExportRangeMode::Clip, start, end};
  return settings;
}
} // namespace

TEST_CASE("Export uses index times and exact ten second frame counts", "[export]")
{
  auto settings = clip();
  for (const auto fps : {30, 60}) {
    settings.fps = fps;
    const ExportTiming timing{settings, snapshot()};
    CHECK(timing.frameCount() == static_cast<std::uint64_t>(fps * 10));
    CHECK(timing.sampleCount() == 480000);
    CHECK(timing.outputDurationSeconds() == 10.0);
    CHECK(timing.frameSourceTimeSeconds(0) == 0.0);
    CHECK(timing.frameSourceTimeSeconds(timing.frameCount() - 1) < 10.0);
    CHECK(timing.sampleSourceTimeSeconds(240000) == 5.0);
    CHECK(timing.frameOutputTimeSeconds(fps * 5) == 5.0);
    CHECK(timing.sampleOutputTimeSeconds(240000) == 5.0);
    CHECK_THROWS_AS(timing.frameSourceTimeSeconds(timing.frameCount()), std::out_of_range);
    CHECK_THROWS_AS(timing.sampleSourceTimeSeconds(timing.sampleCount()), std::out_of_range);
  }
}

TEST_CASE("Export rate scales music and retains a fixed two second full song tail", "[export]")
{
  for (const auto rate : {0.5, 2.0}) {
    const ExportTiming cut{clip(), snapshot(10.0, 0.0, rate)};
    CHECK(cut.frameCount() == (rate == 0.5 ? 1200 : 300));
    CHECK(cut.frameSourceTimeSeconds(60) == rate);
    auto audible = VideoExportSettings{};
    audible.includeAudio = true;
    const ExportTiming full{audible, snapshot(10.0, 0.0, rate)};
    CHECK(full.frameCount() == cut.frameCount() + 120);
    CHECK(full.tailDurationSeconds() == 2.0);
    CHECK(full.frameSourceTimeSeconds(full.frameCount() - 1) == 10.0);
    CHECK(full.sampleSourceTimeSeconds(full.sampleCount() - 1) == 10.0);
    auto silent = VideoExportSettings{};
    silent.includeAudio = false;
    CHECK(ExportTiming{silent, snapshot(10.0, 0.0, rate)}.sampleCount() == full.sampleCount());
  }
}

TEST_CASE("Export preserves preroll and quantizes clip end to one video frame", "[export]")
{
  const ExportTiming timing{clip(1.0, 1.101), snapshot(10.0, -2.0, 2.0)};
  CHECK(timing.frameCount() == 4);
  CHECK(timing.sampleCount() == 3200);
  CHECK(timing.frameSourceTimeSeconds(0) == -1.0);
  CHECK(timing.frameSourceTimeSeconds(3) == Catch::Approx(-0.9));
  CHECK(timing.sampleSourceTimeSeconds(3199) == Catch::Approx(-0.899));
  CHECK(timing.outputDurationSeconds() == Catch::Approx(4.0 / 60.0));
  CHECK(timing.tailDurationSeconds() == 0.0);
  CHECK(timing.musicalDurationSeconds() == Catch::Approx(0.0505));
}

TEST_CASE("Export counts distinguish endpoints on either side of a frame boundary", "[export]")
{
  auto decimal = clip(0.0, 0.1);
  decimal.fps = 30;
  CHECK(ExportTiming{decimal, snapshot()}.frameCount() == 3);
  decimal.range.endSeconds = 1.0 / 30.0;
  CHECK(ExportTiming{decimal, snapshot()}.frameCount() == 1);
  CHECK(ExportTiming{clip(0.0, 1.0), snapshot()}.frameCount() == 60);
  CHECK(ExportTiming{clip(0.0, std::nextafter(1.0, 0.0)), snapshot()}.frameCount() == 60);
  CHECK(ExportTiming{clip(0.0, std::nextafter(1.0, 2.0)), snapshot()}.frameCount() == 61);
  CHECK(ExportTiming{clip(0.0, 0.000001), snapshot()}.sampleCount() == 800);
}

TEST_CASE("Export snapshot survives live edits and timeline replacement", "[export]")
{
  VisualizerController controller;
  auto settings = controller.settings();
  settings.fallingNotes.lookAheadSeconds = 2.0;
  settings.renderer.clearColor.r = 0.25f;
  controller.setSettings(settings);
  MidiTimeline timeline;
  timeline.addNote({.pitch = 60, .velocity = 64, .startSeconds = 5.0, .durationSeconds = 10.0});
  controller.setTimeline(timeline);
  controller.playbackTransport().setEffectiveBpm(120.0, 60.0);
  controller.playbackTransport().seek(4.0);
  const auto frozen = controller.exportSnapshot();
  static_assert(std::is_const_v<std::remove_reference_t<decltype(frozen.timeline)>>);
  settings.fallingNotes.lookAheadSeconds = 7.0;
  settings.renderer.clearColor.r = 0.9f;
  controller.setSettings(settings);
  controller.playbackTransport().setEffectiveBpm(120.0, 240.0);
  controller.setTimeline(std::nullopt);
  CHECK(frozen.timeline.notes().size() == 1);
  CHECK(frozen.sceneConfig.lookAheadSeconds == 2.0);
  CHECK(frozen.background.r == 0.25f);
  CHECK(frozen.timelineOffsetSeconds == 3.0);
  CHECK(frozen.playbackRate == 0.5);
  const ExportTiming timing{VideoExportSettings{}, frozen};
  CHECK(timing.frameCount() == 1560);
  CHECK(timing.frameSourceTimeSeconds(0) == 3.0);
  CHECK_THROWS_AS(controller.exportSnapshot(), std::invalid_argument);
}

TEST_CASE("Export consumes MIDI tempo converted seconds with one frozen speed factor", "[export]")
{
  const auto fixture = midi_fixtures::tempoChangeMidi();
  const auto timeline = MidiFileLoader::loadFromFile(fixture.path());
  REQUIRE(timeline.has_value());
  const ExportSnapshot frozen{*timeline, {}, {}, -0.75, 0.5};
  const ExportTiming timing{clip(0.0, 2.0), frozen};
  CHECK(timing.frameCount() == 240);
  CHECK(timing.frameSourceTimeSeconds(120) == 0.25);
  CHECK(timing.sampleSourceTimeSeconds(96000) == 0.25);
  CHECK(frozen.timeline.sourceBpmAt(timing.frameSourceTimeSeconds(120)) == 60.0);
}

TEST_CASE("Export rejects invalid ranges rates timelines and overflowing counters", "[export]")
{
  const auto nan = std::numeric_limits<double>::quiet_NaN();
  const auto inf = std::numeric_limits<double>::infinity();
  for (const auto range : {ExportRange{ExportRangeMode::Clip, 0.0, 0.0},
                           ExportRange{ExportRangeMode::Clip, 2.0, 1.0},
                           ExportRange{ExportRangeMode::Clip, -1.0, 1.0},
                           ExportRange{ExportRangeMode::Clip, 0.0, 11.0},
                           ExportRange{ExportRangeMode::Clip, nan, 1.0},
                           ExportRange{ExportRangeMode::Clip, 0.0, inf}}) {
    auto settings = clip();
    settings.range = range;
    CHECK_THROWS_AS(ExportTiming(settings, snapshot()), std::invalid_argument);
  }
  for (const auto rate : {0.0, -1.0, nan, inf}) {
    CHECK_THROWS_AS(snapshot(10.0, 0.0, rate), std::invalid_argument);
  }
  CHECK_THROWS_AS(snapshot(10.0, nan), std::invalid_argument);
  CHECK_THROWS_AS(snapshot(10.0, 10.0), std::invalid_argument);
  CHECK_THROWS_AS((ExportSnapshot{MidiTimeline{}, {}, {}, 0.0, 1.0}), std::invalid_argument);
  for (const auto length : {-1.0, nan, inf}) {
    CHECK_THROWS_AS(snapshot(length), std::invalid_argument);
  }
  CHECK_THROWS_AS(ExportTiming(VideoExportSettings{}, snapshot(1e18)), std::overflow_error);
  CHECK_THROWS_AS(ExportTiming(VideoExportSettings{}, snapshot(1e15)), std::overflow_error);
  CHECK_THROWS_AS(ExportTiming(VideoExportSettings{}, snapshot(10.0, 0.0, 1e-300)),
                  std::overflow_error);
}

TEST_CASE("Export rejects malformed notes hidden by a valid cached timeline length", "[export]")
{
  MidiTimeline timeline;
  timeline.addNote({.pitch = 60, .durationSeconds = 10.0});
  timeline.addNote({.pitch = 64,
                    .startSeconds = std::numeric_limits<double>::quiet_NaN(),
                    .durationSeconds = 1.0});
  CHECK_THROWS_AS((ExportSnapshot{timeline, {}, {}, 0.0, 1.0}), std::invalid_argument);
}

TEST_CASE("Export rejects degenerate or nonfinite scene geometry", "[export]")
{
  MidiTimeline timeline;
  timeline.addNote({.pitch = 60, .durationSeconds = 10.0});
  PianoRollSceneConfig config;
  SECTION("zero key width")
  {
    config.keyboardLayout.whiteKeyWidth = 0.0;
  }
  SECTION("zero display height")
  {
    config.displayHeight = 0.0;
  }
  SECTION("nonfinite lookahead")
  {
    config.lookAheadSeconds = std::numeric_limits<double>::infinity();
  }
  CHECK_THROWS_AS((ExportSnapshot{timeline, config, {}, 0.0, 1.0}), std::invalid_argument);
}
