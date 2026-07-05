#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <nlohmann/json.hpp>

#include "app/AppSettingsSerializer.hpp"

namespace {

TEST_CASE("AppSettingsSerializer serializes settings without removed color keys", "[app][settings]")
{
  AppSettings settings;
  settings.renderer.clearColor = Color{.r = 0.1f, .g = 0.2f, .b = 0.3f, .a = 0.4f};
  settings.window.displayMode = WindowDisplayMode::BorderlessFullscreen;
  settings.window.width = 1920;
  settings.window.height = 1200;
  settings.window.vsyncEnabled = false;
  settings.window.fpsLimit = 360;
  settings.playbackControls.seekStepSeconds = 7.5;
  settings.playbackControls.minPlaybackBpm = 45.0;
  settings.playbackControls.maxPlaybackBpm = 220.0;
  settings.playbackControls.playbackBpmStep = 2.0;
  settings.fallingNotes.pitchRange = PitchRange{.minPitch = 30, .maxPitch = 90};
  settings.fallingNotes.noteHorizontalInset = 0.08;
  settings.fallingNotes.blackNoteWidthScale = 0.85;
  settings.fallingNotes.whiteNoteWidthScale = 0.75;
  settings.fallingNotes.outlineColor = Color{.r = 0.9f, .g = 0.8f, .b = 0.7f, .a = 0.6f};
  settings.fallingNotes.outlineThicknessPixels = 3.0;
  settings.fallingNotes.cornerRadiusPixels = 6.5;
  settings.fallingNotes.includeOutline = false;
  settings.keyboard.includeHitLine = false;

  const auto json = nlohmann::json::parse(AppSettingsSerializer::serialize(settings));

  CHECK(json.at("version") == 1);
  CHECK(json.at("window").at("title") == "KeyWave");
  CHECK(json.at("window").at("displayMode") == "borderless_fullscreen");
  CHECK(json.at("window").at("width") == 1920);
  CHECK(json.at("window").at("height") == 1200);
  CHECK(json.at("window").at("vsyncEnabled") == false);
  CHECK(json.at("window").at("fpsLimit") == 360);
  CHECK(json.at("renderer").at("clearColor").at(0) == Catch::Approx(0.1));
  CHECK(json.at("playbackControls").at("seekStepSeconds") == Catch::Approx(7.5));
  CHECK(json.at("playbackControls").at("minPlaybackBpm") == Catch::Approx(45.0));
  CHECK(json.at("playbackControls").at("maxPlaybackBpm") == Catch::Approx(220.0));
  CHECK(json.at("playbackControls").at("playbackBpmStep") == Catch::Approx(2.0));
  CHECK_FALSE(json.at("playbackControls").contains("minPlaybackRate"));
  CHECK_FALSE(json.at("playbackControls").contains("maxPlaybackRate"));
  CHECK_FALSE(json.at("playbackControls").contains("playbackRateStep"));
  CHECK(json.at("fallingNotes").at("pitchRange").at("minPitch") == 30);
  CHECK(json.at("fallingNotes").at("noteHorizontalInset") == Catch::Approx(0.08));
  CHECK(json.at("fallingNotes").at("blackNoteWidthScale") == Catch::Approx(0.85));
  CHECK(json.at("fallingNotes").at("whiteNoteWidthScale") == Catch::Approx(0.75));
  CHECK(json.at("fallingNotes").at("outlineColor").at(0) == Catch::Approx(0.9));
  CHECK(json.at("fallingNotes").at("outlineThicknessPixels") == Catch::Approx(3.0));
  CHECK(json.at("fallingNotes").at("cornerRadiusPixels") == Catch::Approx(6.5));
  CHECK(json.at("fallingNotes").at("includeOutline") == false);
  CHECK(json.at("keyboard").at("includeHitLine") == false);
  CHECK_FALSE(json.at("fallingNotes").contains("clippedNoteColor"));
}

TEST_CASE("AppSettingsSerializer merges missing keys over defaults and ignores unknown keys",
          "[app][settings]")
{
  AppSettings defaults;
  defaults.keyboard.whiteKeyWidth = 1.75;
  defaults.fallingNotes.activeNoteColor = Color{.r = 0.9f, .g = 0.8f, .b = 0.7f, .a = 0.6f};

  const nlohmann::json json = {
    {"version", 1},
    {"unknownTopLevel", true},
    {"renderer", {{"clearColor", {0.5, 0.4, 0.3, 0.2}}, {"ignored", "value"}}},
    {"fallingNotes", {{"clippedNoteColor", {1.0, 0.0, 0.0, 1.0}}}},
  };

  const auto settings = AppSettingsSerializer::deserialize(json.dump(), defaults);

  CHECK(settings.renderer.clearColor.r == Catch::Approx(0.5f));
  CHECK(settings.renderer.clearColor.g == Catch::Approx(0.4f));
  CHECK(settings.renderer.clearColor.b == Catch::Approx(0.3f));
  CHECK(settings.renderer.clearColor.a == Catch::Approx(0.2f));
  CHECK(settings.keyboard.whiteKeyWidth == Catch::Approx(1.75));
  CHECK(settings.fallingNotes.activeNoteColor.r == Catch::Approx(0.9f));
  CHECK(settings.fallingNotes.activeNoteColor.g == Catch::Approx(0.8f));
  CHECK(settings.fallingNotes.activeNoteColor.b == Catch::Approx(0.7f));
  CHECK(settings.fallingNotes.activeNoteColor.a == Catch::Approx(0.6f));
}

TEST_CASE("AppSettingsSerializer falls back for invalid values and clamps colors",
          "[app][settings]")
{
  AppSettings defaults;
  defaults.fallingNotes.pitchRange = PitchRange{.minPitch = 21, .maxPitch = 108};
  defaults.fallingNotes.lookAheadSeconds = 10.0;
  defaults.keyboard.blackKeyWidth = 0.6;
  defaults.keyboard.separatorWidth = 0.015;

  const nlohmann::json json = {
    {"renderer", {{"clearColor", {-1.0, 0.5, 2.0, 1.0}}}},
    {"playbackControls",
     {{"seekStepSeconds", -1.0},
      {"minPlaybackBpm", 300.0},
      {"maxPlaybackBpm", 20.0},
      {"playbackBpmStep", 0.0},
      {"minPlaybackRate", 4.0},
      {"maxPlaybackRate", 0.25},
      {"playbackRateStep", 0.0}}},
    {"fallingNotes",
     {{"pitchRange", {{"minPitch", 120}, {"maxPitch", 20}}},
      {"lookAheadSeconds", 0.0},
      {"visiblePastSeconds", -1.0},
      {"noteHorizontalInset", -0.1},
      {"blackNoteWidthScale", 0.0},
      {"whiteNoteWidthScale", 2.0},
      {"outlineThicknessPixels", -1.0},
      {"cornerRadiusPixels", 25.0},
      {"noteColor", {0.1, 0.2}}}},
    {"keyboard",
     {{"blackKeyWidth", 0.0},
      {"separatorWidth", -1.0},
      {"hitLineHeight", 0.0},
      {"whiteKeyColor", "not-a-color"}}},
  };

  const auto settings = AppSettingsSerializer::deserialize(json.dump(), defaults);

  CHECK(settings.renderer.clearColor.r == Catch::Approx(0.0f));
  CHECK(settings.renderer.clearColor.g == Catch::Approx(0.5f));
  CHECK(settings.renderer.clearColor.b == Catch::Approx(1.0f));
  CHECK(settings.renderer.clearColor.a == Catch::Approx(1.0f));
  CHECK(settings.playbackControls.seekStepSeconds == Catch::Approx(5.0));
  CHECK(settings.playbackControls.minPlaybackBpm == Catch::Approx(20.0));
  CHECK(settings.playbackControls.maxPlaybackBpm == Catch::Approx(300.0));
  CHECK(settings.playbackControls.playbackBpmStep == Catch::Approx(5.0));
  CHECK(settings.fallingNotes.pitchRange.minPitch == 21);
  CHECK(settings.fallingNotes.pitchRange.maxPitch == 108);
  CHECK(settings.fallingNotes.lookAheadSeconds == Catch::Approx(10.0));
  CHECK(settings.fallingNotes.visiblePastSeconds == Catch::Approx(0.0));
  CHECK(settings.fallingNotes.noteHorizontalInset ==
        Catch::Approx(defaults.fallingNotes.noteHorizontalInset));
  CHECK(settings.fallingNotes.blackNoteWidthScale ==
        Catch::Approx(defaults.fallingNotes.blackNoteWidthScale));
  CHECK(settings.fallingNotes.whiteNoteWidthScale ==
        Catch::Approx(defaults.fallingNotes.whiteNoteWidthScale));
  CHECK(settings.fallingNotes.outlineThicknessPixels ==
        Catch::Approx(defaults.fallingNotes.outlineThicknessPixels));
  CHECK(settings.fallingNotes.cornerRadiusPixels ==
        Catch::Approx(defaults.fallingNotes.cornerRadiusPixels));
  CHECK(settings.fallingNotes.noteColor.r == Catch::Approx(defaults.fallingNotes.noteColor.r));
  CHECK(settings.keyboard.blackKeyWidth == Catch::Approx(0.6));
  CHECK(settings.keyboard.separatorWidth == Catch::Approx(0.015));
  CHECK(settings.keyboard.hitLineHeight == Catch::Approx(defaults.keyboard.hitLineHeight));
  CHECK(settings.keyboard.whiteKeyColor.r == Catch::Approx(defaults.keyboard.whiteKeyColor.r));
}

TEST_CASE("AppSettingsSerializer deserializes window settings", "[app][settings]")
{
  const nlohmann::json json = {
    {"window",
     {{"title", "Custom KeyWave"},
      {"displayMode", "exclusive_fullscreen"},
      {"width", 1600},
      {"height", 900},
      {"vsyncEnabled", false},
      {"fpsLimit", 0}}},
  };

  const auto settings = AppSettingsSerializer::deserialize(json.dump());

  CHECK(settings.window.title == "Custom KeyWave");
  CHECK(settings.window.displayMode == WindowDisplayMode::ExclusiveFullscreen);
  CHECK(settings.window.width == 1600);
  CHECK(settings.window.height == 900);
  CHECK_FALSE(settings.window.vsyncEnabled);
  CHECK(settings.window.fpsLimit == unlimitedFpsLimit);
}

TEST_CASE("AppSettingsSerializer falls back for invalid window settings", "[app][settings]")
{
  const nlohmann::json json = {
    {"window",
     {{"title", ""},
      {"displayMode", "not-a-display-mode"},
      {"width", 1000},
      {"height", 1000},
      {"vsyncEnabled", "yes"},
      {"fpsLimit", 75}}},
  };

  const auto settings = AppSettingsSerializer::deserialize(json.dump());

  CHECK(settings.window.title == "KeyWave");
  CHECK(settings.window.displayMode == WindowDisplayMode::Windowed);
  CHECK(settings.window.width == 1280);
  CHECK(settings.window.height == 720);
  CHECK(settings.window.vsyncEnabled);
  CHECK(settings.window.fpsLimit == 60);
}

TEST_CASE("AppSettingsSerializer deserializes falling note outline settings", "[app][settings]")
{
  const nlohmann::json json = {
    {"fallingNotes",
     {{"outlineColor", {0.6, 0.5, 0.4, 0.3}},
      {"outlineThicknessPixels", 4.0},
      {"cornerRadiusPixels", 12.0},
      {"includeOutline", false}}},
  };

  const auto settings = AppSettingsSerializer::deserialize(json.dump());

  CHECK(settings.fallingNotes.outlineColor.r == Catch::Approx(0.6f));
  CHECK(settings.fallingNotes.outlineColor.g == Catch::Approx(0.5f));
  CHECK(settings.fallingNotes.outlineColor.b == Catch::Approx(0.4f));
  CHECK(settings.fallingNotes.outlineColor.a == Catch::Approx(0.3f));
  CHECK(settings.fallingNotes.outlineThicknessPixels == Catch::Approx(4.0));
  CHECK(settings.fallingNotes.cornerRadiusPixels == Catch::Approx(12.0));
  CHECK_FALSE(settings.fallingNotes.includeOutline);
}

TEST_CASE("AppSettingsSerializer deserializes falling note width settings", "[app][settings]")
{
  const nlohmann::json json = {
    {"fallingNotes",
     {{"noteHorizontalInset", 0.12}, {"blackNoteWidthScale", 0.7}, {"whiteNoteWidthScale", 0.8}}},
  };

  const auto settings = AppSettingsSerializer::deserialize(json.dump());

  CHECK(settings.fallingNotes.noteHorizontalInset == Catch::Approx(0.12));
  CHECK(settings.fallingNotes.blackNoteWidthScale == Catch::Approx(0.7));
  CHECK(settings.fallingNotes.whiteNoteWidthScale == Catch::Approx(0.8));
}

TEST_CASE("AppSettingsSerializer clamps pitch values through shared constraints", "[app][settings]")
{
  AppSettings defaults;
  defaults.fallingNotes.pitchRange = PitchRange{.minPitch = 21, .maxPitch = 108};

  const nlohmann::json json = {
    {"fallingNotes", {{"pitchRange", {{"minPitch", -12}, {"maxPitch", 140}}}}},
  };

  const auto settings = AppSettingsSerializer::deserialize(json.dump(), defaults);

  CHECK(settings.fallingNotes.pitchRange.minPitch == 0);
  CHECK(settings.fallingNotes.pitchRange.maxPitch == 127);
}

} // namespace
