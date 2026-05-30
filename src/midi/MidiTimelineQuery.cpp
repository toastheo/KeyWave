#include "midi/MidiTimelineQuery.hpp"

#include <algorithm>
#include <cmath>
#include <sstream>

namespace {

bool isValidTimeRange(const TimeRange& range, DiagnosticSink& diagnostics)
{
  if (!std::isfinite(range.startSeconds) || !std::isfinite(range.endSeconds)) {
    reportWarning(diagnostics,
                  "MIDI timeline query skipped: time range must contain finite values.");
    return false;
  }

  if (range.startSeconds >= range.endSeconds) {
    std::ostringstream message;
    message << "MIDI timeline query skipped: time range start must be before end"
            << " (start=" << range.startSeconds << ", end=" << range.endSeconds << ").";
    reportWarning(diagnostics, message.str());
    return false;
  }

  return true;
}

bool isValidPitchRange(const PitchRange& range, DiagnosticSink& diagnostics)
{
  if (range.minPitch > range.maxPitch) {
    std::ostringstream message;
    message << "MIDI timeline query skipped: pitch range min must be less than or equal to max"
            << " (min=" << range.minPitch << ", max=" << range.maxPitch << ").";
    reportWarning(diagnostics, message.str());
    return false;
  }

  return true;
}

// Treat ranges as open at the boundaries: notes ending exactly at range.startSeconds
// or starting exactly at range.endSeconds are outside the viewport.
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

void sortNotesByPitchChannelTrack(std::vector<Note>& notes)
{
  std::ranges::sort(notes, [](const Note& left, const Note& right) {
    if (left.pitch != right.pitch) {
      return left.pitch < right.pitch;
    }

    if (left.channel != right.channel) {
      return left.channel < right.channel;
    }

    if (left.track != right.track) {
      return left.track < right.track;
    }

    return left.startSeconds < right.startSeconds;
  });
}

bool isActiveAt(const Note& note, const double timeSeconds)
{
  const auto endSeconds = note.startSeconds + note.durationSeconds;
  return std::isfinite(note.startSeconds) && std::isfinite(note.durationSeconds) &&
         std::isfinite(endSeconds) && note.durationSeconds > 0.0 &&
         note.startSeconds <= timeSeconds && endSeconds > timeSeconds;
}

} // namespace

MidiTimelineQuery::MidiTimelineQuery(const MidiTimeline& timeline, DiagnosticSink& diagnostics)
    : m_timeline(timeline)
    , m_diagnostics(&diagnostics)
{}

std::vector<QueriedNote> MidiTimelineQuery::findNotes(const TimelineViewport& viewport) const
{
  if (!isValidTimeRange(viewport.timeRange, *m_diagnostics) ||
      !isValidPitchRange(viewport.pitchRange, *m_diagnostics)) {
    return {};
  }

  std::vector<QueriedNote> result;

  for (const auto& note : m_timeline.notes()) {
    if (!overlapsTimeRange(note, viewport.timeRange) ||
        !isInPitchRange(note, viewport.pitchRange)) {
      continue;
    }

    const bool startsBeforeRange = note.startSeconds < viewport.timeRange.startSeconds;
    const bool endsAfterRange = note.startSeconds + note.durationSeconds >
                                viewport.timeRange.endSeconds;

    result.push_back(QueriedNote{
      .note = note,
      .startsBeforeRange = startsBeforeRange,
      .endsAfterRange = endsAfterRange,
    });
  }

  sortQueriedNotes(result);
  return result;
}

std::vector<QueriedNote> MidiTimelineQuery::findNotesInTimeRange(const TimeRange& range) const
{
  if (!isValidTimeRange(range, *m_diagnostics)) {
    return {};
  }

  std::vector<QueriedNote> result;

  for (const auto& note : m_timeline.notes()) {
    if (!overlapsTimeRange(note, range)) {
      continue;
    }

    const bool startsBeforeRange = note.startSeconds < range.startSeconds;
    const bool endsAfterRange = note.startSeconds + note.durationSeconds > range.endSeconds;

    result.push_back(QueriedNote{
      .note = note,
      .startsBeforeRange = startsBeforeRange,
      .endsAfterRange = endsAfterRange,
    });
  }

  sortQueriedNotes(result);
  return result;
}

std::vector<QueriedNote> MidiTimelineQuery::findNotesInPitchRange(const PitchRange& range) const
{
  if (!isValidPitchRange(range, *m_diagnostics)) {
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

std::vector<Note> MidiTimelineQuery::findActiveNotesAt(const double timeSeconds) const
{
  if (!std::isfinite(timeSeconds) || timeSeconds < 0.0) {
    return {};
  }

  std::vector<Note> result;
  for (const auto& note : m_timeline.notes()) {
    if (isActiveAt(note, timeSeconds)) {
      result.push_back(note);
    }
  }

  sortNotesByPitchChannelTrack(result);
  return result;
}

const MidiTimeline& MidiTimelineQuery::timeline() const
{
  return m_timeline;
}
