#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <string_view>
#include <array>

#include "app/AppSettings.hpp"
#include "app/AppSettingsConstraints.hpp"

namespace {

TEST_CASE("AppSettings defaults preserve the current runtime configuration", "[app][settings]")
{
  const AppSettings settings;

  SECTION("window defaults")
  {
    CHECK(settings.window.title == "KeyWave");
    CHECK(settings.window.width == 1280);
    CHECK(settings.window.height == 720);
    CHECK(settings.window.displayMode == WindowDisplayMode::Windowed);
    CHECK(settings.window.vsyncEnabled);
    CHECK(settings.window.fpsLimit == 60);
  }

  SECTION("renderer defaults")
  {
    CHECK(settings.renderer.clearColor.r == Catch::Approx(0.025f));
    CHECK(settings.renderer.clearColor.g == Catch::Approx(0.03f));
    CHECK(settings.renderer.clearColor.b == Catch::Approx(0.04f));
    CHECK(settings.renderer.clearColor.a == Catch::Approx(1.0f));
  }

  SECTION("playback control defaults")
  {
    CHECK(settings.playbackControls.seekStepSeconds == Catch::Approx(5.0));
    CHECK(settings.playbackControls.minPlaybackRate == Catch::Approx(0.25));
    CHECK(settings.playbackControls.maxPlaybackRate == Catch::Approx(4.0));
    CHECK(settings.playbackControls.playbackRateStep == Catch::Approx(0.25));
  }

  SECTION("falling notes defaults")
  {
    CHECK(settings.fallingNotes.pitchRange.minPitch == 21);
    CHECK(settings.fallingNotes.pitchRange.maxPitch == 108);
    CHECK(settings.fallingNotes.lookAheadSeconds == Catch::Approx(10.0));
    CHECK(settings.fallingNotes.visiblePastSeconds == Catch::Approx(0.0));
    CHECK(settings.fallingNotes.noteHorizontalInset == Catch::Approx(0.04));
    CHECK(settings.fallingNotes.blackNoteWidthScale == Catch::Approx(1.0));
    CHECK(settings.fallingNotes.whiteNoteWidthScale == Catch::Approx(0.92));
    CHECK(settings.fallingNotes.outlineColor.r == Catch::Approx(1.0f));
    CHECK(settings.fallingNotes.outlineColor.g == Catch::Approx(1.0f));
    CHECK(settings.fallingNotes.outlineColor.b == Catch::Approx(1.0f));
    CHECK(settings.fallingNotes.outlineColor.a == Catch::Approx(1.0f));
    CHECK(settings.fallingNotes.outlineThicknessPixels == Catch::Approx(1.0));
    CHECK(settings.fallingNotes.cornerRadiusPixels == Catch::Approx(4.0));
    CHECK(settings.fallingNotes.includeOutline);
  }

  SECTION("keyboard defaults")
  {
    CHECK(settings.keyboard.whiteKeyWidth == Catch::Approx(1.0));
    CHECK(settings.keyboard.whiteKeyHeight == Catch::Approx(2.5));
    CHECK(settings.keyboard.blackKeyWidth == Catch::Approx(0.6));
    CHECK(settings.keyboard.blackKeyHeight == Catch::Approx(1.55));
  }
}

TEST_CASE("AppSettings constraints expose editable setting ranges by group", "[app][settings]")
{
  const auto& constraints = appSettingsConstraints();

  CHECK(constraints.fallingNotes.pitchRange.minimum == 0);
  CHECK(constraints.fallingNotes.pitchRange.maximum == 127);
  CHECK(constraints.fallingNotes.lookAheadSeconds.minimum == Catch::Approx(1.0));
  CHECK(constraints.fallingNotes.lookAheadSeconds.maximum == Catch::Approx(30.0));
  CHECK(constraints.fallingNotes.visiblePastSeconds.minimum == Catch::Approx(0.0));
  CHECK(constraints.fallingNotes.visiblePastSeconds.maximum == Catch::Approx(5.0));
  CHECK(constraints.fallingNotes.noteHorizontalInset.minimum == Catch::Approx(0.0));
  CHECK(constraints.fallingNotes.noteHorizontalInset.maximum == Catch::Approx(0.5));
  CHECK(constraints.fallingNotes.noteWidthScale.minimum == Catch::Approx(0.05));
  CHECK(constraints.fallingNotes.noteWidthScale.maximum == Catch::Approx(1.0));
  CHECK(constraints.fallingNotes.outlineThicknessPixels.minimum == Catch::Approx(0.0));
  CHECK(constraints.fallingNotes.outlineThicknessPixels.maximum == Catch::Approx(8.0));
  CHECK(constraints.fallingNotes.cornerRadiusPixels.minimum == Catch::Approx(0.0));
  CHECK(constraints.fallingNotes.cornerRadiusPixels.maximum == Catch::Approx(24.0));

  CHECK(constraints.keyboard.whiteKeyWidth.minimum == Catch::Approx(0.1));
  CHECK(constraints.keyboard.whiteKeyWidth.maximum == Catch::Approx(3.0));
  CHECK(constraints.keyboard.whiteKeyHeight.minimum == Catch::Approx(0.2));
  CHECK(constraints.keyboard.whiteKeyHeight.maximum == Catch::Approx(6.0));
  CHECK(constraints.keyboard.blackKeyWidth.minimum == Catch::Approx(0.05));
  CHECK(constraints.keyboard.blackKeyHeight.minimum == Catch::Approx(0.1));
  CHECK(constraints.keyboard.separatorWidth.minimum == Catch::Approx(0.0));
  CHECK(constraints.keyboard.separatorWidth.maximum == Catch::Approx(8.0));
  CHECK(constraints.keyboard.hitLineHeight.minimum == Catch::Approx(0.005));
  CHECK(constraints.keyboard.hitLineHeight.maximum == Catch::Approx(0.25));
}

TEST_CASE("Window setting presets expose resolution and frame limit options", "[app][settings]")
{
  const auto resolutions = windowResolutionPresets();
  REQUIRE(resolutions.size() == 4);
  CHECK(resolutions[0].width == 1280);
  CHECK(resolutions[0].height == 720);
  CHECK(std::string_view{resolutions[0].label} == "1280 x 720 (16:9)");
  CHECK(resolutions[3].width == 1920);
  CHECK(resolutions[3].height == 1200);
  CHECK(std::string_view{resolutions[3].label} == "1920 x 1200 (16:10)");

  const auto fpsLimits = windowFpsLimitPresets();
  REQUIRE(fpsLimits.size() == 7);
  CHECK(fpsLimits[0].fpsLimit == unlimitedFpsLimit);
  CHECK(std::string_view{fpsLimits[0].label} == "Unlimited");
  CHECK(fpsLimits[2].fpsLimit == 60);
  CHECK(fpsLimits[6].fpsLimit == 360);
}

TEST_CASE("Window display mode helpers expose labels and setting values", "[app][settings]")
{
  struct DisplayModeTestCase
  {
    WindowDisplayMode mode;
    const char* label;
    const char* settingValue;
  };

  constexpr std::array displayModes{
    DisplayModeTestCase{
      .mode = WindowDisplayMode::Windowed,
      .label = "Windowed",
      .settingValue = "windowed",
    },
    DisplayModeTestCase{
      .mode = WindowDisplayMode::BorderlessFullscreen,
      .label = "Borderless Fullscreen",
      .settingValue = "borderless_fullscreen",
    },
    DisplayModeTestCase{
      .mode = WindowDisplayMode::ExclusiveFullscreen,
      .label = "Exclusive Fullscreen",
      .settingValue = "exclusive_fullscreen",
    },
  };

  for (const auto& displayMode : displayModes) {
    CHECK(std::string_view{windowDisplayModeLabel(displayMode.mode)} == displayMode.label);
    CHECK(std::string_view{windowDisplayModeSettingValue(displayMode.mode)} ==
          displayMode.settingValue);
    CHECK(windowDisplayModeFromSettingValue(displayMode.settingValue) == displayMode.mode);
  }

  CHECK(windowDisplayModeFromSettingValue("unsupported",
                                          WindowDisplayMode::ExclusiveFullscreen) ==
        WindowDisplayMode::ExclusiveFullscreen);
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
  settings.fallingNotes.noteHorizontalInset = -0.1;
  settings.fallingNotes.blackNoteWidthScale = 0.0;
  settings.fallingNotes.whiteNoteWidthScale = 2.0;
  settings.fallingNotes.outlineThicknessPixels = -1.0;
  settings.fallingNotes.cornerRadiusPixels = 25.0;
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
  CHECK(sanitized.fallingNotes.noteHorizontalInset == Catch::Approx(0.04));
  CHECK(sanitized.fallingNotes.blackNoteWidthScale == Catch::Approx(1.0));
  CHECK(sanitized.fallingNotes.whiteNoteWidthScale == Catch::Approx(0.92));
  CHECK(sanitized.fallingNotes.outlineThicknessPixels == Catch::Approx(1.0));
  CHECK(sanitized.fallingNotes.cornerRadiusPixels == Catch::Approx(4.0));
  CHECK(sanitized.keyboard.whiteKeyWidth == Catch::Approx(1.0));
  CHECK(sanitized.keyboard.whiteKeyHeight == Catch::Approx(2.5));
  CHECK(sanitized.keyboard.blackKeyWidth == Catch::Approx(0.6));
  CHECK(sanitized.keyboard.blackKeyHeight == Catch::Approx(1.55));
}

TEST_CASE("sanitizeAppSettings clamps invalid window settings", "[app][settings]")
{
  AppSettings settings;
  settings.window.title.clear();
  settings.window.displayMode = static_cast<WindowDisplayMode>(999);
  settings.window.width = 1234;
  settings.window.height = 567;
  settings.window.fpsLimit = 75;
  settings.window.vsyncEnabled = false;

  const auto sanitized = sanitizeAppSettings(settings);

  CHECK(sanitized.window.title == "KeyWave");
  CHECK(sanitized.window.displayMode == WindowDisplayMode::Windowed);
  CHECK(sanitized.window.width == 1280);
  CHECK(sanitized.window.height == 720);
  CHECK_FALSE(sanitized.window.vsyncEnabled);
  CHECK(sanitized.window.fpsLimit == 60);
}

TEST_CASE("sanitizeAppSettings preserves valid non-default window settings", "[app][settings]")
{
  AppSettings settings;
  settings.window.displayMode = WindowDisplayMode::BorderlessFullscreen;
  settings.window.width = 1920;
  settings.window.height = 1200;
  settings.window.vsyncEnabled = false;
  settings.window.fpsLimit = 144;

  const auto sanitized = sanitizeAppSettings(settings);

  CHECK(sanitized.window.displayMode == WindowDisplayMode::BorderlessFullscreen);
  CHECK(sanitized.window.width == 1920);
  CHECK(sanitized.window.height == 1200);
  CHECK_FALSE(sanitized.window.vsyncEnabled);
  CHECK(sanitized.window.fpsLimit == 144);
}

TEST_CASE(
  "sanitizeAppSettings keeps dependent keyboard dimensions within current parent dimensions",
  "[app][settings]")
{
  AppSettings settings;
  settings.keyboard.whiteKeyWidth = 0.2;
  settings.keyboard.whiteKeyHeight = 0.3;
  settings.keyboard.blackKeyWidth = 0.5;
  settings.keyboard.blackKeyHeight = 0.6;

  const auto sanitized = sanitizeAppSettings(settings);

  CHECK(sanitized.keyboard.whiteKeyWidth == Catch::Approx(0.2));
  CHECK(sanitized.keyboard.whiteKeyHeight == Catch::Approx(0.3));
  CHECK(sanitized.keyboard.blackKeyWidth <= sanitized.keyboard.whiteKeyWidth);
  CHECK(sanitized.keyboard.blackKeyHeight <= sanitized.keyboard.whiteKeyHeight);
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
  CHECK(settings.fallingNotes.lookAheadSeconds ==
        Catch::Approx(defaults.fallingNotes.lookAheadSeconds));
  CHECK(settings.fallingNotes.noteColor.r == Catch::Approx(defaults.fallingNotes.noteColor.r));
  CHECK(settings.keyboard.includeHitLine == defaults.keyboard.includeHitLine);
  CHECK(settings.keyboard.whiteKeyHeight == Catch::Approx(defaults.keyboard.whiteKeyHeight));
}

} // namespace
