#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include "app/AppSettingsSerializer.hpp"

namespace {

TEST_CASE("AppSettingsSerializer serializes settings without removed color keys", "[app][settings]")
{
  AppSettings settings;
  settings.renderer.clearColor = Color{.r = 0.1f, .g = 0.2f, .b = 0.3f, .a = 0.4f};
  settings.playbackControls.seekStepSeconds = 7.5;
  settings.fallingNotes.pitchRange = PitchRange{.minPitch = 30, .maxPitch = 90};
  settings.fallingNotes.noteHorizontalInset = 0.08;
  settings.fallingNotes.blackNoteWidthScale = 0.85;
  settings.fallingNotes.whiteNoteWidthScale = 0.75;
  settings.fallingNotes.outlineColor = Color{.r = 0.9f, .g = 0.8f, .b = 0.7f, .a = 0.6f};
  settings.fallingNotes.outlineThicknessPixels = 3.0;
  settings.fallingNotes.includeOutline = false;
  settings.keyboard.includeHitLine = false;

  const auto json = AppSettingsSerializer::serialize(settings);

  CHECK(json.at("version") == 1);
  CHECK(json.at("renderer").at("clearColor").at(0) == Catch::Approx(0.1));
  CHECK(json.at("playbackControls").at("seekStepSeconds") == Catch::Approx(7.5));
  CHECK(json.at("fallingNotes").at("pitchRange").at("minPitch") == 30);
  CHECK(json.at("fallingNotes").at("noteHorizontalInset") == Catch::Approx(0.08));
  CHECK(json.at("fallingNotes").at("blackNoteWidthScale") == Catch::Approx(0.85));
  CHECK(json.at("fallingNotes").at("whiteNoteWidthScale") == Catch::Approx(0.75));
  CHECK(json.at("fallingNotes").at("outlineColor").at(0) == Catch::Approx(0.9));
  CHECK(json.at("fallingNotes").at("outlineThicknessPixels") == Catch::Approx(3.0));
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

  const auto settings = AppSettingsSerializer::deserialize(json, defaults);

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
      {"noteColor", {0.1, 0.2}}}},
    {"keyboard",
     {{"blackKeyWidth", 0.0},
      {"separatorWidth", -1.0},
      {"hitLineHeight", 0.0},
      {"whiteKeyColor", "not-a-color"}}},
  };

  const auto settings = AppSettingsSerializer::deserialize(json, defaults);

  CHECK(settings.renderer.clearColor.r == Catch::Approx(0.0f));
  CHECK(settings.renderer.clearColor.g == Catch::Approx(0.5f));
  CHECK(settings.renderer.clearColor.b == Catch::Approx(1.0f));
  CHECK(settings.renderer.clearColor.a == Catch::Approx(1.0f));
  CHECK(settings.playbackControls.seekStepSeconds == Catch::Approx(5.0));
  CHECK(settings.playbackControls.minPlaybackRate == Catch::Approx(0.25));
  CHECK(settings.playbackControls.maxPlaybackRate == Catch::Approx(4.0));
  CHECK(settings.playbackControls.playbackRateStep == Catch::Approx(0.25));
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
  CHECK(settings.fallingNotes.noteColor.r == Catch::Approx(defaults.fallingNotes.noteColor.r));
  CHECK(settings.keyboard.blackKeyWidth == Catch::Approx(0.6));
  CHECK(settings.keyboard.separatorWidth == Catch::Approx(0.015));
  CHECK(settings.keyboard.hitLineHeight == Catch::Approx(defaults.keyboard.hitLineHeight));
  CHECK(settings.keyboard.whiteKeyColor.r == Catch::Approx(defaults.keyboard.whiteKeyColor.r));
}

TEST_CASE("AppSettingsSerializer deserializes falling note outline settings", "[app][settings]")
{
  const nlohmann::json json = {
    {"fallingNotes",
     {{"outlineColor", {0.6, 0.5, 0.4, 0.3}},
      {"outlineThicknessPixels", 4.0},
      {"includeOutline", false}}},
  };

  const auto settings = AppSettingsSerializer::deserialize(json);

  CHECK(settings.fallingNotes.outlineColor.r == Catch::Approx(0.6f));
  CHECK(settings.fallingNotes.outlineColor.g == Catch::Approx(0.5f));
  CHECK(settings.fallingNotes.outlineColor.b == Catch::Approx(0.4f));
  CHECK(settings.fallingNotes.outlineColor.a == Catch::Approx(0.3f));
  CHECK(settings.fallingNotes.outlineThicknessPixels == Catch::Approx(4.0));
  CHECK_FALSE(settings.fallingNotes.includeOutline);
}

TEST_CASE("AppSettingsSerializer deserializes falling note width settings", "[app][settings]")
{
  const nlohmann::json json = {
    {"fallingNotes",
     {{"noteHorizontalInset", 0.12}, {"blackNoteWidthScale", 0.7}, {"whiteNoteWidthScale", 0.8}}},
  };

  const auto settings = AppSettingsSerializer::deserialize(json);

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

  const auto settings = AppSettingsSerializer::deserialize(json, defaults);

  CHECK(settings.fallingNotes.pitchRange.minPitch == 0);
  CHECK(settings.fallingNotes.pitchRange.maxPitch == 127);
}

} // namespace
