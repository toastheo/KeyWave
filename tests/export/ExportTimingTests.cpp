#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <cmath>
#include <cstdint>
#include <limits>
#include <optional>
#include <stdexcept>
#include <type_traits>

#include "app/VisualizerController.hpp"
#include "export/ExportSnapshot.hpp"
#include "export/ExportTiming.hpp"
#include "export/VideoExportSettings.hpp"
#include "midi/MidiFileLoader.hpp"
#include "midi/MidiFixtures.hpp"
#include "midi/MidiTimeline.hpp"

namespace {
struct SnapshotOptions
{
  double length = 10.0;
  double offset = 0.0;
  double rate = 1.0;
};

ExportSnapshot snapshot(SnapshotOptions options = {})
{
  const auto [length, offset, rate] = options;
  MidiTimeline timeline;
  timeline.addNote({.pitch = 60, .velocity = 64, .startSeconds = 0.0, .durationSeconds = length});
  return ExportSnapshot{timeline, {}, {}, offset, rate};
}

VideoExportSettings clip(double start = 0.0, double end = 10.0)
{
  VideoExportSettings settings;
  settings.range = {.mode = ExportRangeMode::Clip, .startSeconds = start, .endSeconds = end};
  return settings;
}
} // namespace

namespace {

TEST_CASE("Export uses index times and exact ten second frame counts", "[export]")
{
  auto settings = clip();
  for (const auto fps : {30, 60}) {
    settings.fps = fps;
    const ExportTiming timing{settings, snapshot()};
    CHECK(timing.frameCount() == static_cast<std::uint64_t>(fps) * 10);
    CHECK(timing.sampleCount() == 480000);
    CHECK(timing.outputDurationSeconds() == 10.0);
    CHECK(timing.frameSourceTimeSeconds(0) == 0.0);
    CHECK(timing.frameSourceTimeSeconds(timing.frameCount() - 1) < 10.0);
    CHECK(timing.sampleSourceTimeSeconds(240000) == 5.0);
    CHECK(timing.frameOutputTimeSeconds(static_cast<std::uint64_t>(fps) * 5) == 5.0);
    CHECK(timing.sampleOutputTimeSeconds(240000) == 5.0);
    CHECK_THROWS_AS(timing.frameSourceTimeSeconds(timing.frameCount()), std::out_of_range);
    CHECK_THROWS_AS(timing.sampleSourceTimeSeconds(timing.sampleCount()), std::out_of_range);
  }
}

TEST_CASE("Export rate scales music and retains a fixed two second full song tail", "[export]")
{
  for (const auto rate : {0.5, 2.0}) {
    const ExportTiming cut{clip(), snapshot({.length = 10.0, .offset = 0.0, .rate = rate})};
    CHECK(cut.frameCount() == (rate == 0.5 ? 1200 : 300));
    CHECK(cut.frameSourceTimeSeconds(60) == rate);
    auto audible = VideoExportSettings{};
    audible.includeAudio = true;
    const ExportTiming full{audible, snapshot({.length = 10.0, .offset = 0.0, .rate = rate})};
    CHECK(full.frameCount() == cut.frameCount() + 120);
    CHECK(full.tailDurationSeconds() == 2.0);
    CHECK(full.frameSourceTimeSeconds(full.frameCount() - 1) == 10.0);
    CHECK(full.sampleSourceTimeSeconds(full.sampleCount() - 1) == 10.0);
    auto silent = VideoExportSettings{};
    silent.includeAudio = false;
    CHECK(
      ExportTiming{silent, snapshot({.length = 10.0, .offset = 0.0, .rate = rate})}.sampleCount() ==
      full.sampleCount());
  }
}

TEST_CASE("Export preserves preroll and quantizes clip end to one video frame", "[export]")
{
  const ExportTiming timing{clip(1.0, 1.101),
                            snapshot({.length = 10.0, .offset = -2.0, .rate = 2.0})};
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
  for (const auto range :
       {ExportRange{.mode = ExportRangeMode::Clip, .startSeconds = 0.0, .endSeconds = 0.0},
        ExportRange{.mode = ExportRangeMode::Clip, .startSeconds = 2.0, .endSeconds = 1.0},
        ExportRange{.mode = ExportRangeMode::Clip, .startSeconds = -1.0, .endSeconds = 1.0},
        ExportRange{.mode = ExportRangeMode::Clip, .startSeconds = 0.0, .endSeconds = 11.0},
        ExportRange{.mode = ExportRangeMode::Clip, .startSeconds = nan, .endSeconds = 1.0},
        ExportRange{.mode = ExportRangeMode::Clip, .startSeconds = 0.0, .endSeconds = inf}}) {
    auto settings = clip();
    settings.range = range;
    CHECK_THROWS_AS(ExportTiming(settings, snapshot()), std::invalid_argument);
  }
  for (const auto rate : {0.0, -1.0, nan, inf}) {
    CHECK_THROWS_AS(snapshot({.length = 10.0, .offset = 0.0, .rate = rate}), std::invalid_argument);
  }
  CHECK_THROWS_AS(snapshot({.length = 10.0, .offset = nan}), std::invalid_argument);
  CHECK_THROWS_AS(snapshot({.length = 10.0, .offset = 10.0}), std::invalid_argument);
  CHECK_THROWS_AS((ExportSnapshot{MidiTimeline{}, {}, {}, 0.0, 1.0}), std::invalid_argument);
  for (const auto length : {-1.0, nan, inf}) {
    CHECK_THROWS_AS(snapshot({.length = length}), std::invalid_argument);
  }
  CHECK_THROWS_AS(ExportTiming(VideoExportSettings{}, snapshot({.length = 1e18})),
                  std::overflow_error);
  CHECK_THROWS_AS(ExportTiming(VideoExportSettings{}, snapshot({.length = 1e15})),
                  std::overflow_error);
  CHECK_THROWS_AS(ExportTiming(VideoExportSettings{},
                               snapshot({.length = 10.0, .offset = 0.0, .rate = 1e-300})),
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

TEST_CASE("Export snapshot accepts normalized colors and rejects out of range channels", "[export]")
{
  MidiTimeline timeline;
  timeline.addNote({.pitch = 60, .durationSeconds = 10.0});
  for (const auto channel : {0.0f, 0.5f, 1.0f}) {
    CHECK_NOTHROW((ExportSnapshot{timeline, {}, {channel, channel, channel, channel}, 0.0, 1.0}));
  }
  // Use otherwise-valid all-white styles so rejection specifically exercises
  // the supplied channel, rather than another default color.
  PianoRollSceneConfig config;
  config.fallingNotesStyle.noteColor = {.r = 1, .g = 1, .b = 1, .a = 1};
  config.fallingNotesStyle.activeNoteColor = {.r = 1, .g = 1, .b = 1, .a = 1};
  config.fallingNotesStyle.outlineColor = {.r = 1, .g = 1, .b = 1, .a = 1};
  config.keyboardStyle.whiteKeyColor = {.r = 1, .g = 1, .b = 1, .a = 1};
  config.keyboardStyle.blackKeyColor = {.r = 1, .g = 1, .b = 1, .a = 1};
  config.keyboardStyle.activeWhiteKeyColor = {.r = 1, .g = 1, .b = 1, .a = 1};
  config.keyboardStyle.activeBlackKeyColor = {.r = 1, .g = 1, .b = 1, .a = 1};
  config.keyboardStyle.whiteKeySeparatorColor = {.r = 1, .g = 1, .b = 1, .a = 1};
  config.keyboardStyle.hitLineColor = {.r = 1, .g = 1, .b = 1, .a = 1};
  for (const auto channel : {-0.1f,
                             1.1f,
                             std::numeric_limits<float>::infinity(),
                             std::numeric_limits<float>::quiet_NaN()}) {
    CHECK_THROWS_AS((ExportSnapshot{timeline, config, {channel, 1, 1, 1}, 0.0, 1.0}),
                    std::invalid_argument);
  }
}

} // namespace
