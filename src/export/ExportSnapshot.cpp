#include "export/ExportSnapshot.hpp"

#include <cmath>
#include <stdexcept>
#include <utility>

namespace {
void requireFiniteNonnegative(double value)
{
  if (!std::isfinite(value) || value < 0.0) {
    throw std::invalid_argument("Export source values must be finite or nonnegative");
  }
}
} // namespace

ExportSnapshot::ExportSnapshot(MidiTimeline sourceTimeline,
                               PianoRollSceneConfig config,
                               Color sourceBackground,
                               double offset,
                               double rate)
    : timeline(std::move(sourceTimeline))
    , sceneConfig(std::move(config))
    , background(sourceBackground)
    , timelineOffsetSeconds(offset)
    , playbackRate(rate)
{
  if (timeline.empty() || !std::isfinite(offset) || !std::isfinite(rate) || rate <= 0.0 ||
      !std::isfinite(playbackDurationSeconds()) ||playbackDurationSeconds() <= 0.0) {
    throw std::invalid_argument(
      "Export requires a nonempty timeline, finite offset and positive rate/duration.");
  }
  for (const auto& note : timeline.notes()) {
    requireFiniteNonnegative(note.startSeconds);
    requireFiniteNonnegative(note.durationSeconds);
    requireFiniteNonnegative(note.startSeconds + note.durationSeconds);
    if (note.pitch < 0 || note.pitch > 127 || note.velocity < 0 || note.velocity > 127 ||
        note.channel < 0 || note.channel > 15) {
      throw std::invalid_argument("Invalid MIDI note in export timeline.");
    }
  }
  for (const auto& range : {sceneConfig.pitchRange, sceneConfig.keyboardLayout.pitchRange}) {
    if (range.minPitch < 0 || range.maxPitch > 127 || range.minPitch > range.maxPitch) {
      throw std::invalid_argument("Invalid export pitch range.");
    }
  }
  const auto& keyboard = sceneConfig.keyboardLayout;
  const auto& notes = sceneConfig.fallingNotesStyle;
  const auto& keys = sceneConfig.keyboardStyle;
  if (sceneConfig.displayHeight <= 0.0 || keyboard.whiteKeyWidth <= 0.0) {
    throw std::invalid_argument("Export display height and white key width must be positive.");
  }
  for (const auto value : {sceneConfig.lookAheadSeconds,
                           sceneConfig.visiblePastSeconds,
                           sceneConfig.displayHeight,
                           keyboard.whiteKeyWidth,
                           keyboard.whiteKeyHeight,
                           keyboard.blackKeyWidth,
                           keyboard.blackKeyHeight,
                           keyboard.whiteKeyGap,
                           sceneConfig.fallingNotesLayout.noteHorizontalInset,
                           sceneConfig.fallingNotesLayout.blackNoteWidthScale,
                           sceneConfig.fallingNotesLayout.whiteNoteWidthScale,
                           notes.outlineThicknessPixels,
                           notes.cornerRadiusPixels,
                           keys.separatorThicknessPixels,
                           keys.hitLineHeight}) {
    requireFiniteNonnegative(value);
  }
  for (const auto color : {background,
                           notes.noteColor,
                           notes.activeNoteColor,
                           notes.outlineColor,
                           keys.whiteKeyColor,
                           keys.blackKeyColor,
                           keys.activeWhiteKeyColor,
                           keys.activeBlackKeyColor,
                           keys.whiteKeySeparatorColor,
                           keys.hitLineColor}) {
    for (const auto component : {color.r, color.g, color.b, color.a}) {
      requireFiniteNonnegative(component);
      if (component > 1.0f) {
        throw std::invalid_argument("Export colors must be in [0, 1].");
      }
    }
  }
}

double ExportSnapshot::playbackDurationSeconds() const
{
  return timeline.lengthSeconds() - timelineOffsetSeconds;
}

