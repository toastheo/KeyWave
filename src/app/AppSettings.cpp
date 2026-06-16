#include "app/AppSettings.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <span>
#include <string_view>

#include "app/AppSettingsConstraints.hpp"

namespace {

template <typename Settings> Settings defaultsFor()
{
  return Settings{};
}

constexpr auto windowResolutions = std::array{
  WindowResolutionPreset{.width = 1280, .height = 720, .label = "1280 x 720 (16:9)"},
  WindowResolutionPreset{.width = 1600, .height = 900, .label = "1600 x 900 (16:9)"},
  WindowResolutionPreset{.width = 1920, .height = 1080, .label = "1920 x 1080 (16:9)"},
  WindowResolutionPreset{.width = 1920, .height = 1200, .label = "1920 x 1200 (16:10)"},
};

constexpr auto windowFpsLimits = std::array{
  WindowFpsLimitPreset{.fpsLimit = unlimitedFpsLimit, .label = "Unlimited"},
  WindowFpsLimitPreset{.fpsLimit = 30, .label = "30"},
  WindowFpsLimitPreset{.fpsLimit = 60, .label = "60"},
  WindowFpsLimitPreset{.fpsLimit = 120, .label = "120"},
  WindowFpsLimitPreset{.fpsLimit = 144, .label = "144"},
  WindowFpsLimitPreset{.fpsLimit = 240, .label = "240"},
  WindowFpsLimitPreset{.fpsLimit = 360, .label = "360"},
};

bool isKnownDisplayMode(const WindowDisplayMode displayMode)
{
  switch (displayMode) {
    case WindowDisplayMode::Windowed:
    case WindowDisplayMode::BorderlessFullscreen:
    case WindowDisplayMode::ExclusiveFullscreen:
      return true;
  }
  return false;
}

bool isPositiveFinite(const double value)
{
  return std::isfinite(value) && value > 0.0;
}

bool isNonNegativeFinite(const double value)
{
  return std::isfinite(value) && value >= 0.0;
}

bool isFiniteInRange(const double value, const DoubleSettingRange range)
{
  return std::isfinite(value) && value >= range.minimum && value <= range.maximum;
}

double defaultWithinRange(const double fallback, const DoubleSettingRange range)
{
  return std::clamp(fallback, range.minimum, range.maximum);
}

bool isValidPitchRange(const PitchRange& range)
{
  const auto pitchConstraints = appSettingsConstraints().fallingNotes.pitchRange;
  return range.minPitch >= pitchConstraints.minimum && range.maxPitch <= pitchConstraints.maximum &&
         range.minPitch <= range.maxPitch;
}

} // namespace

std::span<const WindowResolutionPreset> windowResolutionPresets()
{
  return windowResolutions;
}

std::span<const WindowFpsLimitPreset> windowFpsLimitPresets()
{
  return windowFpsLimits;
}

const char* windowDisplayModeLabel(const WindowDisplayMode displayMode)
{
  switch (displayMode) {
    case WindowDisplayMode::Windowed:
      return "Windowed";
    case WindowDisplayMode::BorderlessFullscreen:
      return "Borderless Fullscreen";
    case WindowDisplayMode::ExclusiveFullscreen:
      return "Exclusive Fullscreen";
  }
  return "Windowed";
}

const char* windowDisplayModeSettingValue(const WindowDisplayMode displayMode)
{
  switch (displayMode) {
    case WindowDisplayMode::Windowed:
      return "windowed";
    case WindowDisplayMode::BorderlessFullscreen:
      return "borderless_fullscreen";
    case WindowDisplayMode::ExclusiveFullscreen:
      return "exclusive_fullscreen";
  }
  return "windowed";
}

WindowDisplayMode windowDisplayModeFromSettingValue(const std::string_view value,
                                                    const WindowDisplayMode fallback)
{
  if (value == "windowed") {
    return WindowDisplayMode::Windowed;
  }
  if (value == "borderless_fullscreen") {
    return WindowDisplayMode::BorderlessFullscreen;
  }
  if (value == "exclusive_fullscreen") {
    return WindowDisplayMode::ExclusiveFullscreen;
  }
  return fallback;
}

bool isSupportedWindowResolution(const int width, const int height)
{
  return std::ranges::any_of(windowResolutions,
                             [width, height](const WindowResolutionPreset& preset) {
                               return preset.width == width && preset.height == height;
                             });
}

bool isSupportedWindowFpsLimit(const int fpsLimit)
{
  return std::ranges::any_of(windowFpsLimits, [fpsLimit](const WindowFpsLimitPreset& preset) {
    return preset.fpsLimit == fpsLimit;
  });
}

WindowSettings sanitizeWindowSettings(WindowSettings settings)
{
  const auto defaults = defaultsFor<WindowSettings>();

  if (settings.title.empty()) {
    settings.title = defaults.title;
  }
  if (!isKnownDisplayMode(settings.displayMode)) {
    settings.displayMode = defaults.displayMode;
  }
  if (!isSupportedWindowResolution(settings.width, settings.height)) {
    settings.width = defaults.width;
    settings.height = defaults.height;
  }
  if (!isSupportedWindowFpsLimit(settings.fpsLimit)) {
    settings.fpsLimit = defaults.fpsLimit;
  }

  return settings;
}

PlaybackControlSettings sanitizePlaybackControlSettings(PlaybackControlSettings settings)
{
  const auto defaults = defaultsFor<PlaybackControlSettings>();

  if (!isPositiveFinite(settings.seekStepSeconds)) {
    settings.seekStepSeconds = defaults.seekStepSeconds;
  }
  if (!isPositiveFinite(settings.minPlaybackRate) || !isPositiveFinite(settings.maxPlaybackRate) ||
      settings.minPlaybackRate > settings.maxPlaybackRate) {
    settings.minPlaybackRate = defaults.minPlaybackRate;
    settings.maxPlaybackRate = defaults.maxPlaybackRate;
  }
  if (!isPositiveFinite(settings.playbackRateStep)) {
    settings.playbackRateStep = defaults.playbackRateStep;
  }

  return settings;
}

FallingNotesSettings sanitizeFallingNotesSettings(FallingNotesSettings settings)
{
  const auto defaults = defaultsFor<FallingNotesSettings>();
  const auto constraints = appSettingsConstraints().fallingNotes;

  if (!isValidPitchRange(settings.pitchRange)) {
    settings.pitchRange = defaults.pitchRange;
  }
  if (!isFiniteInRange(settings.lookAheadSeconds, constraints.lookAheadSeconds)) {
    settings.lookAheadSeconds = defaults.lookAheadSeconds;
  }
  if (!isFiniteInRange(settings.visiblePastSeconds, constraints.visiblePastSeconds)) {
    settings.visiblePastSeconds = defaults.visiblePastSeconds;
  }
  if (!isFiniteInRange(settings.noteHorizontalInset, constraints.noteHorizontalInset)) {
    settings.noteHorizontalInset = defaults.noteHorizontalInset;
  }
  if (!isFiniteInRange(settings.blackNoteWidthScale, constraints.noteWidthScale)) {
    settings.blackNoteWidthScale = defaults.blackNoteWidthScale;
  }
  if (!isFiniteInRange(settings.whiteNoteWidthScale, constraints.noteWidthScale)) {
    settings.whiteNoteWidthScale = defaults.whiteNoteWidthScale;
  }
  if (!isFiniteInRange(settings.outlineThicknessPixels, constraints.outlineThicknessPixels)) {
    settings.outlineThicknessPixels = defaults.outlineThicknessPixels;
  }
  if (!isFiniteInRange(settings.cornerRadiusPixels, constraints.cornerRadiusPixels)) {
    settings.cornerRadiusPixels = defaults.cornerRadiusPixels;
  }

  return settings;
}

KeyboardSettings sanitizeKeyboardSettings(KeyboardSettings settings)
{
  const auto defaults = defaultsFor<KeyboardSettings>();
  const auto constraints = appSettingsConstraints().keyboard;

  if (!isFiniteInRange(settings.whiteKeyWidth, constraints.whiteKeyWidth)) {
    settings.whiteKeyWidth = defaults.whiteKeyWidth;
  }
  if (!isFiniteInRange(settings.whiteKeyHeight, constraints.whiteKeyHeight)) {
    settings.whiteKeyHeight = defaults.whiteKeyHeight;
  }
  if (!std::isfinite(settings.blackKeyWidth) ||
      settings.blackKeyWidth < constraints.blackKeyWidth.minimum ||
      settings.blackKeyWidth > settings.whiteKeyWidth) {
    settings.blackKeyWidth =
      defaultWithinRange(defaults.blackKeyWidth,
                         DoubleSettingRange{.minimum = constraints.blackKeyWidth.minimum,
                                            .maximum = settings.whiteKeyWidth});
  }
  if (!std::isfinite(settings.blackKeyHeight) ||
      settings.blackKeyHeight < constraints.blackKeyHeight.minimum ||
      settings.blackKeyHeight > settings.whiteKeyHeight) {
    settings.blackKeyHeight =
      defaultWithinRange(defaults.blackKeyHeight,
                         DoubleSettingRange{.minimum = constraints.blackKeyHeight.minimum,
                                            .maximum = settings.whiteKeyHeight});
  }
  if (!isNonNegativeFinite(settings.whiteKeyGap)) {
    settings.whiteKeyGap = defaults.whiteKeyGap;
  }
  // Leave a tiny width so extreme gaps cannot create zero-width note geometry downstream.
  settings.whiteKeyGap =
    std::min(settings.whiteKeyGap, std::max(0.0, settings.whiteKeyWidth - 0.000001));

  if (!isFiniteInRange(settings.separatorWidth, constraints.separatorWidth)) {
    settings.separatorWidth = defaults.separatorWidth;
  }
  if (!isFiniteInRange(settings.hitLineHeight, constraints.hitLineHeight)) {
    settings.hitLineHeight = defaults.hitLineHeight;
  }

  return settings;
}

AppSettings sanitizeAppSettings(AppSettings settings)
{
  settings.window = sanitizeWindowSettings(settings.window);
  settings.playbackControls = sanitizePlaybackControlSettings(settings.playbackControls);
  settings.fallingNotes = sanitizeFallingNotesSettings(settings.fallingNotes);
  settings.keyboard = sanitizeKeyboardSettings(settings.keyboard);
  return settings;
}

void resetAppSettingsToDefaults(AppSettings& settings)
{
  settings = sanitizeAppSettings(AppSettings{});
}
