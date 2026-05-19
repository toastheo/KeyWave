#include "app/AppSettings.hpp"

#include <algorithm>
#include <cmath>

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

bool isValidPitchRange(const PitchRange& range)
{
  return range.minPitch <= range.maxPitch;
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

  if (!isValidPitchRange(settings.pitchRange)) {
    settings.pitchRange = defaults.pitchRange;
  }
  if (!isNonNegativeFinite(settings.lookAheadSeconds)) {
    settings.lookAheadSeconds = defaults.lookAheadSeconds;
  }
  if (!isNonNegativeFinite(settings.visiblePastSeconds)) {
    settings.visiblePastSeconds = defaults.visiblePastSeconds;
  }

  return settings;
}

KeyboardSettings sanitizeKeyboardSettings(KeyboardSettings settings)
{
  const auto defaults = defaultsFor<KeyboardSettings>();

  if (!isPositiveFinite(settings.whiteKeyWidth)) {
    settings.whiteKeyWidth = defaults.whiteKeyWidth;
  }
  if (!isPositiveFinite(settings.whiteKeyHeight)) {
    settings.whiteKeyHeight = defaults.whiteKeyHeight;
  }
  if (!isPositiveFinite(settings.blackKeyWidth)) {
    settings.blackKeyWidth = defaults.blackKeyWidth;
  }
  if (!isPositiveFinite(settings.blackKeyHeight)) {
    settings.blackKeyHeight = defaults.blackKeyHeight;
  }
  if (!isNonNegativeFinite(settings.whiteKeyGap)) {
    settings.whiteKeyGap = defaults.whiteKeyGap;
  }
  settings.whiteKeyGap =
    std::min(settings.whiteKeyGap, std::max(0.0, settings.whiteKeyWidth - 0.000001));

  if (!isPositiveFinite(settings.separatorWidth)) {
    settings.separatorWidth = defaults.separatorWidth;
  }
  if (!isPositiveFinite(settings.hitLineHeight)) {
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
