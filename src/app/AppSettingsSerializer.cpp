#include "app/AppSettingsSerializer.hpp"

#include <algorithm>
#include <cmath>

#include "app/AppSettingsConstraints.hpp"

namespace {

constexpr int settingsSchemaVersion = 1;

nlohmann::json colorToJson(const Color color)
{
  return nlohmann::json::array({color.r, color.g, color.b, color.a});
}

float clampColorChannel(const double value)
{
  return static_cast<float>(std::clamp(value, 0.0, 1.0));
}

Color colorFromJson(const nlohmann::json& json, const Color fallback)
{
  if (!json.is_array() || json.size() < 4) {
    return fallback;
  }

  for (std::size_t index = 0; index < 4; ++index) {
    if (!json.at(index).is_number()) {
      return fallback;
    }
  }

  return Color{
    .r = clampColorChannel(json.at(0).get<double>()),
    .g = clampColorChannel(json.at(1).get<double>()),
    .b = clampColorChannel(json.at(2).get<double>()),
    .a = clampColorChannel(json.at(3).get<double>()),
  };
}

nlohmann::json pitchRangeToJson(const PitchRange range)
{
  return nlohmann::json{{"minPitch", range.minPitch}, {"maxPitch", range.maxPitch}};
}

PitchRange pitchRangeFromJson(const nlohmann::json& json, const PitchRange fallback)
{
  if (!json.is_object()) {
    return fallback;
  }

  PitchRange range = fallback;
  const auto constraints = appSettingsConstraints().fallingNotes.pitchRange;
  if (const auto iter = json.find("minPitch"); iter != json.end() && iter->is_number_integer()) {
    range.minPitch = std::clamp(iter->get<int>(), constraints.minimum, constraints.maximum);
  }
  if (const auto iter = json.find("maxPitch"); iter != json.end() && iter->is_number_integer()) {
    range.maxPitch = std::clamp(iter->get<int>(), constraints.minimum, constraints.maximum);
  }

  if (range.minPitch > range.maxPitch) {
    return fallback;
  }

  return range;
}

bool isPositiveFinite(const double value)
{
  return std::isfinite(value) && value > 0.0;
}

bool isNonNegativeFinite(const double value)
{
  return std::isfinite(value) && value >= 0.0;
}

double numberOrFallback(const nlohmann::json& object,
                        const char* key,
                        const double fallback,
                        const bool requirePositive)
{
  const auto iter = object.find(key);
  if (iter == object.end() || !iter->is_number()) {
    return fallback;
  }

  const double value = iter->get<double>();
  if (requirePositive ? isPositiveFinite(value) : isNonNegativeFinite(value)) {
    return value;
  }

  return fallback;
}

double numberInRangeOrFallback(const nlohmann::json& object,
                               const char* key,
                               const double fallback,
                               const DoubleSettingRange range)
{
  const auto iter = object.find(key);
  if (iter == object.end() || !iter->is_number()) {
    return fallback;
  }

  const double value = iter->get<double>();
  if (std::isfinite(value) && value >= range.minimum && value <= range.maximum) {
    return value;
  }

  return fallback;
}

bool boolOrFallback(const nlohmann::json& object, const char* key, const bool fallback)
{
  const auto iter = object.find(key);
  if (iter == object.end() || !iter->is_boolean()) {
    return fallback;
  }
  return iter->get<bool>();
}

void deserializeWindowSettings(const nlohmann::json& json, WindowSettings& settings)
{
  if (!json.is_object()) {
    return;
  }

  if (const auto iter = json.find("title"); iter != json.end() && iter->is_string()) {
    settings.title = iter->get<std::string>();
  }
  if (const auto iter = json.find("displayMode"); iter != json.end() && iter->is_string()) {
    settings.displayMode =
      windowDisplayModeFromSettingValue(iter->get<std::string>(), settings.displayMode);
  }
  if (const auto iter = json.find("width"); iter != json.end() && iter->is_number_integer()) {
    settings.width = iter->get<int>();
  }
  if (const auto iter = json.find("height"); iter != json.end() && iter->is_number_integer()) {
    settings.height = iter->get<int>();
  }
  settings.vsyncEnabled = boolOrFallback(json, "vsyncEnabled", settings.vsyncEnabled);
  if (const auto iter = json.find("fpsLimit"); iter != json.end() && iter->is_number_integer()) {
    settings.fpsLimit = iter->get<int>();
  }
}

void deserializeRendererSettings(const nlohmann::json& json, RendererSettings& settings)
{
  if (!json.is_object()) {
    return;
  }

  if (const auto iter = json.find("clearColor"); iter != json.end()) {
    settings.clearColor = colorFromJson(*iter, settings.clearColor);
  }
}

void deserializePlaybackControlSettings(const nlohmann::json& json,
                                        PlaybackControlSettings& settings)
{
  if (!json.is_object()) {
    return;
  }

  settings.seekStepSeconds =
    numberOrFallback(json, "seekStepSeconds", settings.seekStepSeconds, true);
  settings.minPlaybackRate =
    numberOrFallback(json, "minPlaybackRate", settings.minPlaybackRate, true);
  settings.maxPlaybackRate =
    numberOrFallback(json, "maxPlaybackRate", settings.maxPlaybackRate, true);
  settings.playbackRateStep =
    numberOrFallback(json, "playbackRateStep", settings.playbackRateStep, true);

  if (settings.minPlaybackRate > settings.maxPlaybackRate) {
    constexpr PlaybackControlSettings defaults;
    settings.minPlaybackRate = defaults.minPlaybackRate;
    settings.maxPlaybackRate = defaults.maxPlaybackRate;
  }
}

void deserializeFallingNotesSettings(const nlohmann::json& json, FallingNotesSettings& settings)
{
  if (!json.is_object()) {
    return;
  }

  if (const auto iter = json.find("pitchRange"); iter != json.end()) {
    settings.pitchRange = pitchRangeFromJson(*iter, settings.pitchRange);
  }
  const auto constraints = appSettingsConstraints().fallingNotes;
  settings.lookAheadSeconds = numberInRangeOrFallback(
    json, "lookAheadSeconds", settings.lookAheadSeconds, constraints.lookAheadSeconds);
  settings.visiblePastSeconds = numberInRangeOrFallback(
    json, "visiblePastSeconds", settings.visiblePastSeconds, constraints.visiblePastSeconds);
  settings.noteHorizontalInset = numberInRangeOrFallback(
    json, "noteHorizontalInset", settings.noteHorizontalInset, constraints.noteHorizontalInset);
  settings.blackNoteWidthScale = numberInRangeOrFallback(
    json, "blackNoteWidthScale", settings.blackNoteWidthScale, constraints.noteWidthScale);
  settings.whiteNoteWidthScale = numberInRangeOrFallback(
    json, "whiteNoteWidthScale", settings.whiteNoteWidthScale, constraints.noteWidthScale);
  if (const auto iter = json.find("noteColor"); iter != json.end()) {
    settings.noteColor = colorFromJson(*iter, settings.noteColor);
  }
  if (const auto iter = json.find("activeNoteColor"); iter != json.end()) {
    settings.activeNoteColor = colorFromJson(*iter, settings.activeNoteColor);
  }
  if (const auto iter = json.find("outlineColor"); iter != json.end()) {
    settings.outlineColor = colorFromJson(*iter, settings.outlineColor);
  }
  settings.outlineThicknessPixels = numberInRangeOrFallback(json,
                                                            "outlineThicknessPixels",
                                                            settings.outlineThicknessPixels,
                                                            constraints.outlineThicknessPixels);
  settings.cornerRadiusPixels = numberInRangeOrFallback(
    json, "cornerRadiusPixels", settings.cornerRadiusPixels, constraints.cornerRadiusPixels);
  settings.includeOutline = boolOrFallback(json, "includeOutline", settings.includeOutline);
}

void deserializeKeyboardSettings(const nlohmann::json& json, KeyboardSettings& settings)
{
  if (!json.is_object()) {
    return;
  }

  const auto constraints = appSettingsConstraints().keyboard;
  settings.whiteKeyWidth = numberInRangeOrFallback(
    json, "whiteKeyWidth", settings.whiteKeyWidth, constraints.whiteKeyWidth);
  settings.whiteKeyHeight = numberInRangeOrFallback(
    json, "whiteKeyHeight", settings.whiteKeyHeight, constraints.whiteKeyHeight);
  settings.blackKeyWidth = numberOrFallback(json, "blackKeyWidth", settings.blackKeyWidth, true);
  settings.blackKeyHeight = numberOrFallback(json, "blackKeyHeight", settings.blackKeyHeight, true);
  settings.whiteKeyGap = numberOrFallback(json, "whiteKeyGap", settings.whiteKeyGap, false);

  if (const auto iter = json.find("whiteKeyColor"); iter != json.end()) {
    settings.whiteKeyColor = colorFromJson(*iter, settings.whiteKeyColor);
  }
  if (const auto iter = json.find("blackKeyColor"); iter != json.end()) {
    settings.blackKeyColor = colorFromJson(*iter, settings.blackKeyColor);
  }
  if (const auto iter = json.find("activeWhiteKeyColor"); iter != json.end()) {
    settings.activeWhiteKeyColor = colorFromJson(*iter, settings.activeWhiteKeyColor);
  }
  if (const auto iter = json.find("activeBlackKeyColor"); iter != json.end()) {
    settings.activeBlackKeyColor = colorFromJson(*iter, settings.activeBlackKeyColor);
  }
  if (const auto iter = json.find("whiteKeySeparatorColor"); iter != json.end()) {
    settings.whiteKeySeparatorColor = colorFromJson(*iter, settings.whiteKeySeparatorColor);
  }
  if (const auto iter = json.find("hitLineColor"); iter != json.end()) {
    settings.hitLineColor = colorFromJson(*iter, settings.hitLineColor);
  }

  settings.separatorWidth = numberInRangeOrFallback(
    json, "separatorWidth", settings.separatorWidth, constraints.separatorWidth);
  settings.hitLineHeight = numberInRangeOrFallback(
    json, "hitLineHeight", settings.hitLineHeight, constraints.hitLineHeight);
  settings.includeSeparators =
    boolOrFallback(json, "includeSeparators", settings.includeSeparators);
  settings.includeHitLine = boolOrFallback(json, "includeHitLine", settings.includeHitLine);
}

} // namespace

nlohmann::json AppSettingsSerializer::serialize(const AppSettings& settings)
{
  return nlohmann::json{
    {"version", settingsSchemaVersion},
    {"window",
     {{"title", settings.window.title},
      {"displayMode", windowDisplayModeSettingValue(settings.window.displayMode)},
      {"width", settings.window.width},
      {"height", settings.window.height},
      {"vsyncEnabled", settings.window.vsyncEnabled},
      {"fpsLimit", settings.window.fpsLimit}}},
    {"renderer", {{"clearColor", colorToJson(settings.renderer.clearColor)}}},
    {"playbackControls",
     {{"seekStepSeconds", settings.playbackControls.seekStepSeconds},
      {"minPlaybackRate", settings.playbackControls.minPlaybackRate},
      {"maxPlaybackRate", settings.playbackControls.maxPlaybackRate},
      {"playbackRateStep", settings.playbackControls.playbackRateStep}}},
    {"fallingNotes",
     {{"pitchRange", pitchRangeToJson(settings.fallingNotes.pitchRange)},
      {"lookAheadSeconds", settings.fallingNotes.lookAheadSeconds},
      {"visiblePastSeconds", settings.fallingNotes.visiblePastSeconds},
      {"noteHorizontalInset", settings.fallingNotes.noteHorizontalInset},
      {"blackNoteWidthScale", settings.fallingNotes.blackNoteWidthScale},
      {"whiteNoteWidthScale", settings.fallingNotes.whiteNoteWidthScale},
      {"noteColor", colorToJson(settings.fallingNotes.noteColor)},
      {"activeNoteColor", colorToJson(settings.fallingNotes.activeNoteColor)},
      {"outlineColor", colorToJson(settings.fallingNotes.outlineColor)},
      {"outlineThicknessPixels", settings.fallingNotes.outlineThicknessPixels},
      {"cornerRadiusPixels", settings.fallingNotes.cornerRadiusPixels},
      {"includeOutline", settings.fallingNotes.includeOutline}}},
    {"keyboard",
     {{"whiteKeyWidth", settings.keyboard.whiteKeyWidth},
      {"whiteKeyHeight", settings.keyboard.whiteKeyHeight},
      {"blackKeyWidth", settings.keyboard.blackKeyWidth},
      {"blackKeyHeight", settings.keyboard.blackKeyHeight},
      {"whiteKeyGap", settings.keyboard.whiteKeyGap},
      {"whiteKeyColor", colorToJson(settings.keyboard.whiteKeyColor)},
      {"blackKeyColor", colorToJson(settings.keyboard.blackKeyColor)},
      {"activeWhiteKeyColor", colorToJson(settings.keyboard.activeWhiteKeyColor)},
      {"activeBlackKeyColor", colorToJson(settings.keyboard.activeBlackKeyColor)},
      {"whiteKeySeparatorColor", colorToJson(settings.keyboard.whiteKeySeparatorColor)},
      {"hitLineColor", colorToJson(settings.keyboard.hitLineColor)},
      {"separatorWidth", settings.keyboard.separatorWidth},
      {"hitLineHeight", settings.keyboard.hitLineHeight},
      {"includeSeparators", settings.keyboard.includeSeparators},
      {"includeHitLine", settings.keyboard.includeHitLine}}},
  };
}

AppSettings AppSettingsSerializer::deserialize(const nlohmann::json& json,
                                               const AppSettings& defaults)
{
  // Merge onto defaults so older or hand-edited settings files can omit fields safely.
  AppSettings settings = defaults;
  if (!json.is_object()) {
    return sanitizeAppSettings(settings);
  }

  if (const auto iter = json.find("window"); iter != json.end()) {
    deserializeWindowSettings(*iter, settings.window);
  }
  if (const auto iter = json.find("renderer"); iter != json.end()) {
    deserializeRendererSettings(*iter, settings.renderer);
  }
  if (const auto iter = json.find("playbackControls"); iter != json.end()) {
    deserializePlaybackControlSettings(*iter, settings.playbackControls);
  }
  if (const auto iter = json.find("fallingNotes"); iter != json.end()) {
    deserializeFallingNotesSettings(*iter, settings.fallingNotes);
  }
  if (const auto iter = json.find("keyboard"); iter != json.end()) {
    deserializeKeyboardSettings(*iter, settings.keyboard);
  }

  return sanitizeAppSettings(settings);
}
