#include "pianoroll/PianoRollLayout.hpp"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <limits>

namespace {

constexpr double kMinimumVisibleNoteHeight = 0.000001;

bool isValidTimeRange(const TimeRange& range)
{
  if (!std::isfinite(range.startSeconds) || !std::isfinite(range.endSeconds)) {
    std::cerr << "Piano roll layout skipped: time range must contain finite values.\n";
    return false;
  }

  if (range.startSeconds >= range.endSeconds) {
    std::cerr << "Piano roll layout skipped: time range start must be before end"
              << " (start=" << range.startSeconds << ", end=" << range.endSeconds << ").\n";
    return false;
  }

  return true;
}

bool isValidPitchRange(const PitchRange& range)
{
  if (range.minPitch > range.maxPitch) {
    std::cerr << "Piano roll layout skipped: pitch range min must be less than or equal to max"
              << " (min=" << range.minPitch << ", max=" << range.maxPitch << ").\n";
    return false;
  }

  return true;
}

int pitchLaneCountForRange(const PitchRange& range)
{
  const auto laneCount =
      static_cast<long long>(range.maxPitch) - static_cast<long long>(range.minPitch) + 1;
  if (laneCount <= 0 || laneCount > std::numeric_limits<int>::max()) {
    return 0;
  }

  return static_cast<int>(laneCount);
}

double sanitizedNoteHeight(const PianoRollLayoutConfig& config)
{
  if (!std::isfinite(config.noteHeight) || config.noteHeight <= 0.0) {
    return kMinimumVisibleNoteHeight;
  }

  return std::clamp(config.noteHeight, kMinimumVisibleNoteHeight, 1.0);
}

double noteLayoutHeight(const PianoRollLayoutConfig& config)
{
  const auto noteHeight = sanitizedNoteHeight(config);
  const auto gap = std::isfinite(config.noteVerticalGap) ? std::max(0.0, config.noteVerticalGap)
                                                         : 0.0;
  return std::max(kMinimumVisibleNoteHeight, noteHeight - gap);
}

double noteEndSeconds(const Note& note)
{
  return note.startSeconds + note.durationSeconds;
}

} // namespace

bool PianoRollLayoutResult::empty() const
{
  return notes.empty();
}

PianoRollLayoutResult PianoRollLayout::build(const std::vector<QueriedNote>& queriedNotes,
                                             const PianoRollLayoutViewport& viewport,
                                             const PianoRollLayoutConfig& config)
{
  PianoRollLayoutResult result{
      .timeRange = viewport.timeRange,
      .pitchRange = viewport.pitchRange,
  };

  if (!isValidTimeRange(viewport.timeRange) || !isValidPitchRange(viewport.pitchRange)) {
    return result;
  }

  const auto pitchLaneCount = pitchLaneCountForRange(viewport.pitchRange);
  if (pitchLaneCount <= 0) {
    std::cerr << "Piano roll layout skipped: pitch lane count must be positive.\n";
    return result;
  }

  const auto viewportDuration = viewport.timeRange.endSeconds - viewport.timeRange.startSeconds;
  const auto height = noteLayoutHeight(config);

  result.pitchLaneCount = pitchLaneCount;
  result.contentWidth = 1.0;
  result.contentHeight = static_cast<double>(pitchLaneCount);
  result.notes.reserve(queriedNotes.size());

  for (const auto& queriedNote : queriedNotes) {
    const auto& note = queriedNote.note;
    const auto noteEnd = noteEndSeconds(note);
    if (!std::isfinite(note.startSeconds) || !std::isfinite(noteEnd)) {
      continue;
    }

    const auto visibleStart = std::max(note.startSeconds, viewport.timeRange.startSeconds);
    const auto visibleEnd = std::min(noteEnd, viewport.timeRange.endSeconds);
    const auto visibleDuration = visibleEnd - visibleStart;
    if (visibleDuration <= 0.0) {
      continue;
    }

    const auto pitchLane = note.pitch - viewport.pitchRange.minPitch;
    if (pitchLane < 0 || pitchLane >= pitchLaneCount) {
      continue;
    }

    result.notes.push_back(PianoRollNoteLayout{
        .note = note,
        .x = (visibleStart - viewport.timeRange.startSeconds) / viewportDuration,
        .y = static_cast<double>(pitchLane),
        .width = visibleDuration / viewportDuration,
        .height = height,
        .visibleStartSeconds = visibleStart,
        .visibleEndSeconds = visibleEnd,
        .clippedLeft = note.startSeconds<viewport.timeRange.startSeconds, .clippedRight = noteEnd>
                           viewport.timeRange.endSeconds,
    });
  }

  return result;
}
