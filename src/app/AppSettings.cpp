#include "app/AppSettings.hpp"

#include <algorithm>
#include <cmath>

#include "app/AppSettingsConstraints.hpp"

namespace {

template <typename Settings> Settings defaultsFor()
{
  return Settings{};
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

WindowSettings sanitizeWindowSettings(WindowSettings settings)
{
  const auto defaults = defaultsFor<WindowSettings>();

  if (settings.title.empty()) {
    settings.title = defaults.title;
  }
  if (settings.width <= 0) {
    settings.width = defaults.width;
  }
  if (settings.height <= 0) {
    settings.height = defaults.height;
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
