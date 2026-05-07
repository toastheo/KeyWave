#include "midi/MidiTimelineQuery.hpp"

#include <algorithm>
#include <cmath>
#include <iostream>

namespace {

bool isValidTimeRange(const TimeRange& range)
{
  if (!std::isfinite(range.startSeconds) || !std::isfinite(range.endSeconds)) {
    std::cerr << "MIDI timeline query skipped: time range must contain finite values.\n";
    return false;
  }

  if (range.startSeconds >= range.endSeconds) {
    std::cerr << "MIDI timeline query skipped: time range start must be before end"
              << " (start=" << range.startSeconds << ", end=" << range.endSeconds << ").\n";
    return false;
  }

  return true;
}

bool isValidPitchRange(const PitchRange& range)
{
  if (range.minPitch > range.maxPitch) {
    std::cerr << "MIDI timeline query skipped: pitch range min must be less than or equal to max"
              << " (min=" << range.minPitch << ", max=" << range.maxPitch << ").\n";
    return false;
  }

  return true;
}

bool overlapsTimeRange(const Note& note, const TimeRange& range)
{
  return note.startSeconds < range.endSeconds &&
         note.startSeconds + note.durationSeconds > range.startSeconds;
}

bool isInPitchRange(const Note& note, const PitchRange& range)
{
  return note.pitch >= range.minPitch && note.pitch <= range.maxPitch;
}

void sortQueriedNotes(std::vector<QueriedNote>& notes)
{
  std::ranges::sort(notes, [](const QueriedNote& left, const QueriedNote& right) {
    if (left.note.startSeconds != right.note.startSeconds) {
      return left.note.startSeconds < right.note.startSeconds;
    }

    if (left.note.pitch != right.note.pitch) {
      return left.note.pitch < right.note.pitch;
    }

    if (left.note.channel != right.note.channel) {
      return left.note.channel < right.note.channel;
    }

    return left.note.track < right.note.track;
  });
}

} // namespace

MidiTimelineQuery::MidiTimelineQuery(const MidiTimeline& timeline)
    : m_timeline(timeline)
{}

std::vector<QueriedNote> MidiTimelineQuery::findNotes(const TimelineViewport& viewport) const
{
  if (!isValidTimeRange(viewport.timeRange) || !isValidPitchRange(viewport.pitchRange)) {
    return {};
  }

  std::vector<QueriedNote> result;

  for (const auto& note : m_timeline.notes()) {
    if (!overlapsTimeRange(note, viewport.timeRange) ||
        !isInPitchRange(note, viewport.pitchRange)) {
      continue;
    }

    result.push_back(QueriedNote{
      .note = note,
      .startsBeforeRange =
        note
          .startSeconds<viewport.timeRange.startSeconds,
                        .endsAfterRange = note.startSeconds + note.durationSeconds> viewport
          .timeRange.endSeconds,
    });
  }

  sortQueriedNotes(result);
  return result;
}

std::vector<QueriedNote> MidiTimelineQuery::findNotesInTimeRange(const TimeRange& range) const
{
  if (!isValidTimeRange(range)) {
    return {};
  }

  std::vector<QueriedNote> result;

  for (const auto& note : m_timeline.notes()) {
    if (!overlapsTimeRange(note, range)) {
      continue;
    }

    result.push_back(QueriedNote{
      .note = note,
      .startsBeforeRange =
        note
          .startSeconds<range.startSeconds,
                        .endsAfterRange = note.startSeconds + note.durationSeconds> range.endSeconds,
    });
  }

  sortQueriedNotes(result);
  return result;
}

std::vector<QueriedNote> MidiTimelineQuery::findNotesInPitchRange(const PitchRange& range) const
{
  if (!isValidPitchRange(range)) {
    return {};
  }

  std::vector<QueriedNote> result;

  for (const auto& note : m_timeline.notes()) {
    if (!isInPitchRange(note, range)) {
      continue;
    }

    result.push_back(QueriedNote{
      .note = note,
      .startsBeforeRange = false,
      .endsAfterRange = false,
    });
  }

  sortQueriedNotes(result);
  return result;
}

const MidiTimeline& MidiTimelineQuery::timeline() const
{
  return m_timeline;
}
