#include "app/AppSettings.hpp"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

namespace {

TEST_CASE("AppSettings defaults preserve the current runtime configuration", "[app][settings]")
{
  const AppSettings settings;

  CHECK(settings.window.title == "KeyWave");
  CHECK(settings.window.width == 1280);
  CHECK(settings.window.height == 720);

  CHECK(settings.renderer.clearColor.r == Catch::Approx(0.025f));
  CHECK(settings.renderer.clearColor.g == Catch::Approx(0.03f));
  CHECK(settings.renderer.clearColor.b == Catch::Approx(0.04f));
  CHECK(settings.renderer.clearColor.a == Catch::Approx(1.0f));

  CHECK(settings.playbackControls.seekStepSeconds == Catch::Approx(5.0));
  CHECK(settings.playbackControls.minPlaybackRate == Catch::Approx(0.25));
  CHECK(settings.playbackControls.maxPlaybackRate == Catch::Approx(4.0));
  CHECK(settings.playbackControls.playbackRateStep == Catch::Approx(0.25));

  CHECK(settings.fallingNotes.pitchRange.minPitch == 21);
  CHECK(settings.fallingNotes.pitchRange.maxPitch == 108);
  CHECK(settings.fallingNotes.lookAheadSeconds == Catch::Approx(10.0));
  CHECK(settings.fallingNotes.visiblePastSeconds == Catch::Approx(0.0));

  CHECK(settings.keyboard.whiteKeyWidth == Catch::Approx(1.0));
  CHECK(settings.keyboard.whiteKeyHeight == Catch::Approx(2.5));
  CHECK(settings.keyboard.blackKeyWidth == Catch::Approx(0.6));
  CHECK(settings.keyboard.blackKeyHeight == Catch::Approx(1.55));
}

TEST_CASE("sanitizeAppSettings clamps invalid customizable values", "[app][settings]")
{
  AppSettings settings;
  settings.window.width = 0;
  settings.window.height = -10;
  settings.playbackControls.seekStepSeconds = -5.0;
  settings.playbackControls.minPlaybackRate = 4.0;
  settings.playbackControls.maxPlaybackRate = 0.25;
  settings.playbackControls.playbackRateStep = 0.0;
  settings.fallingNotes.pitchRange = PitchRange{.minPitch = 108, .maxPitch = 21};
  settings.fallingNotes.lookAheadSeconds = -10.0;
  settings.fallingNotes.visiblePastSeconds = -0.5;
  settings.keyboard.whiteKeyWidth = 0.0;
  settings.keyboard.whiteKeyHeight = -1.0;
  settings.keyboard.blackKeyWidth = 0.0;
  settings.keyboard.blackKeyHeight = -0.62;

  const auto sanitized = sanitizeAppSettings(settings);

  CHECK(sanitized.window.width == 1280);
  CHECK(sanitized.window.height == 720);
  CHECK(sanitized.playbackControls.seekStepSeconds == Catch::Approx(5.0));
  CHECK(sanitized.playbackControls.minPlaybackRate == Catch::Approx(0.25));
  CHECK(sanitized.playbackControls.maxPlaybackRate == Catch::Approx(4.0));
  CHECK(sanitized.playbackControls.playbackRateStep == Catch::Approx(0.25));
  CHECK(sanitized.fallingNotes.pitchRange.minPitch == 21);
  CHECK(sanitized.fallingNotes.pitchRange.maxPitch == 108);
  CHECK(sanitized.fallingNotes.lookAheadSeconds == Catch::Approx(10.0));
  CHECK(sanitized.fallingNotes.visiblePastSeconds == Catch::Approx(0.0));
  CHECK(sanitized.keyboard.whiteKeyWidth == Catch::Approx(1.0));
  CHECK(sanitized.keyboard.whiteKeyHeight == Catch::Approx(2.5));
  CHECK(sanitized.keyboard.blackKeyWidth == Catch::Approx(0.6));
  CHECK(sanitized.keyboard.blackKeyHeight == Catch::Approx(1.55));
}

TEST_CASE("resetAppSettingsToDefaults restores all settings defaults", "[app][settings]")
{
  AppSettings settings;
  settings.renderer.clearColor = Color{.r = 1.0f, .g = 0.0f, .b = 0.0f, .a = 1.0f};
  settings.fallingNotes.lookAheadSeconds = 24.0;
  settings.fallingNotes.noteColor = Color{.r = 0.9f, .g = 0.8f, .b = 0.7f, .a = 0.6f};
  settings.keyboard.includeHitLine = false;
  settings.keyboard.whiteKeyHeight = 4.0;

  resetAppSettingsToDefaults(settings);

  const AppSettings defaults;
  CHECK(settings.renderer.clearColor.r == Catch::Approx(defaults.renderer.clearColor.r));
  CHECK(settings.renderer.clearColor.g == Catch::Approx(defaults.renderer.clearColor.g));
  CHECK(settings.renderer.clearColor.b == Catch::Approx(defaults.renderer.clearColor.b));
  CHECK(settings.renderer.clearColor.a == Catch::Approx(defaults.renderer.clearColor.a));
  CHECK(settings.fallingNotes.lookAheadSeconds == Catch::Approx(defaults.fallingNotes.lookAheadSeconds));
  CHECK(settings.fallingNotes.noteColor.r == Catch::Approx(defaults.fallingNotes.noteColor.r));
  CHECK(settings.keyboard.includeHitLine == defaults.keyboard.includeHitLine);
  CHECK(settings.keyboard.whiteKeyHeight == Catch::Approx(defaults.keyboard.whiteKeyHeight));
}

} // namespace
