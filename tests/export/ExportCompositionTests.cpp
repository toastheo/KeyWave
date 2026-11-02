#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <stdexcept>
#include <variant> // NOLINT(misc-include-cleaner): std::get for RenderCommand variants.

#include "core/CoreTypes.hpp"
#include "export/ExportComposition.hpp"
#include "fallingnotes/PianoRollSceneBuilder.hpp"
#include "keyboard/KeyboardGeometry.hpp"
#include "midi/MidiTimeline.hpp"
#include "render/RenderCommand.hpp"

namespace {
TEST_CASE("Export and preview scenes share aspect-correct geometry and scale pixel styles",
          "[export]")
{
  MidiTimeline timeline;
  timeline.addNote({.pitch = 60, .velocity = 80, .startSeconds = 1, .durationSeconds = 1});
  PianoRollSceneConfig config;
  config.fallingNotesStyle.outlineThicknessPixels = 6;
  const auto full = buildExportScene(timeline, 0, config, {.width = 1080, .height = 1920});
  const auto preview = buildExportScene(timeline, 0, config, {.width = 180, .height = 320});
  CHECK(full.view.visibleWorldRect.width / full.view.visibleWorldRect.height ==
        Catch::Approx(9.0 / 16));
  CHECK(preview.view.visibleWorldRect.height == full.view.visibleWorldRect.height);
  REQUIRE(full.commands.size() == preview.commands.size());
  const auto& note = std::get<DrawStyledRectCommand>(full.commands.front());
  const auto& small = std::get<DrawStyledRectCommand>(preview.commands.front());
  CHECK(note.rect.x == small.rect.x);
  CHECK(note.rect.y == small.rect.y);
  CHECK(note.rect.height == small.rect.height);
  CHECK(note.borderThicknessPixels == 6);
  CHECK(small.borderThicknessPixels == Catch::Approx(1));
}
TEST_CASE("Export includes selected black keys at both range edges", "[export]")
{
  for (const auto range :
       {PitchRange{.minPitch = 61, .maxPitch = 73}, PitchRange{.minPitch = 61, .maxPitch = 61}}) {
    PianoRollSceneConfig config;
    config.pitchRange = range;
    const auto composition = exportComposition(config, {.width = 1080, .height = 1920});
    const KeyboardGeometry keys(composition.sceneConfig.keyboardLayout);
    if (range.minPitch == range.maxPitch) {
      CHECK(keys.height() == config.keyboardLayout.blackKeyHeight);
      CHECK(composition.view.visibleWorldRect.y == -config.keyboardLayout.blackKeyHeight);
    }
    const auto& view = composition.view.visibleWorldRect;
    for (int pitch = range.minPitch; pitch <= range.maxPitch; ++pitch) {
      REQUIRE(keys.containsPitch(pitch));
      const auto rect = keys.keyRectForPitch(pitch);
      CHECK(rect.x >= view.x);
      CHECK(rect.x + rect.width <= view.x + view.width);
    }
    CHECK_FALSE(keys.containsPitch(range.minPitch - 1));
    CHECK_FALSE(keys.containsPitch(range.maxPitch + 1));
  }
}
TEST_CASE("Export composition preserves key proportions and fits every selected key", "[export]")
{
  PianoRollSceneConfig config;
  config.pitchRange = {.minPitch = 60, .maxPitch = 72};
  config.keyboardLayout.whiteKeyHeight = 4.0;
  for (const auto size : {FramebufferSize{.width = 1280, .height = 720},
                          FramebufferSize{.width = 1080, .height = 1920},
                          FramebufferSize{.width = 2160, .height = 2160}}) {
    const auto composition = exportComposition(config, size);
    const KeyboardGeometry keys(composition.sceneConfig.keyboardLayout);
    const auto& view = composition.view.visibleWorldRect;
    CHECK(view.width / view.height == Catch::Approx(double(size.width) / size.height));
    CHECK(view.x <= 0.0);
    CHECK(view.x + view.width >= keys.width());
    CHECK(view.y == Catch::Approx(-keys.height()));
    CHECK(composition.sceneConfig.displayHeight == Catch::Approx(view.height - keys.height()));
    CHECK(composition.sceneConfig.lookAheadSeconds == config.lookAheadSeconds);
    CHECK(composition.sceneConfig.keyboardLayout.whiteKeyHeight == 4.0);
    CHECK(composition.sceneConfig.pitchRange.minPitch == 60);
    CHECK(composition.sceneConfig.pitchRange.maxPitch == 72);
  }
}

TEST_CASE("Export composition scales pixel styles from a 1080 short-side reference", "[export]")
{
  PianoRollSceneConfig config;
  config.fallingNotesStyle.outlineThicknessPixels = 3;
  config.fallingNotesStyle.cornerRadiusPixels = 9;
  config.keyboardStyle.separatorThicknessPixels = 3;
  const auto portrait = exportComposition(config, {.width = 2160, .height = 3840});
  CHECK(portrait.sceneConfig.fallingNotesStyle.outlineThicknessPixels == 6);
  CHECK(portrait.sceneConfig.fallingNotesStyle.cornerRadiusPixels == 18);
  CHECK(portrait.sceneConfig.keyboardStyle.separatorThicknessPixels == 6);
  const auto preview = exportComposition(config, {.width = 180, .height = 320});
  CHECK(preview.view.visibleWorldRect.width == portrait.view.visibleWorldRect.width);
  CHECK(preview.view.visibleWorldRect.height == portrait.view.visibleWorldRect.height);
  CHECK(preview.sceneConfig.fallingNotesStyle.cornerRadiusPixels == Catch::Approx(1.5));
  CHECK_THROWS_AS(exportComposition(config, {.width = 0, .height = 720}), std::invalid_argument);
}
} // namespace
