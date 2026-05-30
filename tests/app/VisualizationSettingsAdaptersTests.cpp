#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <limits>

#include "app/VisualizationSettingsAdapters.hpp"

namespace {

void checkColor(const Color& actual, const Color& expected)
{
  CHECK(actual.r == Catch::Approx(expected.r));
  CHECK(actual.g == Catch::Approx(expected.g));
  CHECK(actual.b == Catch::Approx(expected.b));
  CHECK(actual.a == Catch::Approx(expected.a));
}

TEST_CASE("Visualization settings adapters sanitize app settings into piano-roll scene config",
          "[app][settings]")
{
  constexpr FallingNotesSettings fallingNotesSettings{
    .pitchRange = PitchRange{.minPitch = 60, .maxPitch = 64},
    .lookAheadSeconds = 4.0,
    .visiblePastSeconds = 1.0,
    .noteColor = Color{.r = 0.7f, .g = 0.2f, .b = 0.1f, .a = 1.0f},
    .activeNoteColor = Color{.r = 0.1f, .g = 0.9f, .b = 0.2f, .a = 1.0f},
  };
  constexpr KeyboardSettings keyboardSettings{
    .whiteKeyWidth = 2.0,
    .whiteKeyHeight = 1.25,
    .blackKeyWidth = 1.0,
    .blackKeyHeight = 0.75,
    .whiteKeyGap = 0.2,
    .hitLineColor = Color{.r = 0.6f, .g = 0.6f, .b = 0.9f, .a = 1.0f},
    .hitLineHeight = 0.05,
  };

  const auto config = pianoRollSceneConfigFromSettings(fallingNotesSettings, keyboardSettings);

  CHECK(config.pitchRange.minPitch == 60);
  CHECK(config.pitchRange.maxPitch == 64);
  CHECK(config.lookAheadSeconds == Catch::Approx(4.0));
  CHECK(config.visiblePastSeconds == Catch::Approx(1.0));
  CHECK(config.keyboardLayout.pitchRange.minPitch == 60);
  CHECK(config.keyboardLayout.pitchRange.maxPitch == 64);
  CHECK(config.keyboardLayout.whiteKeyWidth == Catch::Approx(2.0));
  CHECK(config.keyboardLayout.whiteKeyHeight == Catch::Approx(1.25));
  CHECK(config.keyboardLayout.blackKeyWidth == Catch::Approx(1.0));
  CHECK(config.keyboardLayout.blackKeyHeight == Catch::Approx(0.75));
  CHECK(config.keyboardLayout.whiteKeyGap == Catch::Approx(0.2));
  checkColor(config.fallingNotesStyle.noteColor, fallingNotesSettings.noteColor);
  checkColor(config.fallingNotesStyle.activeNoteColor, fallingNotesSettings.activeNoteColor);
  CHECK(config.keyboardStyle.hitLineHeight == Catch::Approx(0.05));
  checkColor(config.keyboardStyle.hitLineColor, keyboardSettings.hitLineColor);
}

TEST_CASE("Visualization settings adapters fall back from invalid settings",
          "[app][settings]")
{
  FallingNotesSettings fallingNotesSettings;
  fallingNotesSettings.pitchRange = PitchRange{.minPitch = 108, .maxPitch = 21};
  fallingNotesSettings.lookAheadSeconds = std::numeric_limits<double>::quiet_NaN();

  KeyboardSettings keyboardSettings;
  keyboardSettings.whiteKeyWidth = -1.0;

  const auto config = pianoRollSceneConfigFromSettings(fallingNotesSettings, keyboardSettings);

  CHECK(config.pitchRange.minPitch == FallingNotesSettings{}.pitchRange.minPitch);
  CHECK(config.pitchRange.maxPitch == FallingNotesSettings{}.pitchRange.maxPitch);
  CHECK(config.lookAheadSeconds == Catch::Approx(FallingNotesSettings{}.lookAheadSeconds));
  CHECK(config.keyboardLayout.whiteKeyWidth == Catch::Approx(KeyboardSettings{}.whiteKeyWidth));
}

} // namespace
