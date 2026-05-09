#pragma once

#include <vector>

#include "midi/MidiTimeline.hpp"
#include "midi/MidiTypes.hpp"

struct TimeRange
{
  double startSeconds = 0.0;
  double endSeconds = 0.0;
};

struct TimelineViewport
{
  TimeRange timeRange;
  PitchRange pitchRange;
};

struct QueriedNote
{
  Note note;
  bool startsBeforeRange = false;
  bool endsAfterRange = false;
};

/**
 * Renderer-independent query helper for selecting notes from a MidiTimeline
 * by time and pitch ranges.
 */
class MidiTimelineQuery
{
public:
  explicit MidiTimelineQuery(const MidiTimeline& timeline);

  [[nodiscard]] std::vector<QueriedNote> findNotes(const TimelineViewport& viewport) const;
  [[nodiscard]] std::vector<QueriedNote> findNotesInTimeRange(const TimeRange& range) const;
  [[nodiscard]] std::vector<QueriedNote> findNotesInPitchRange(const PitchRange& range) const;
  [[nodiscard]] std::vector<Note> findActiveNotesAt(double timeSeconds) const;

  [[nodiscard]] const MidiTimeline& timeline() const;

private:
  const MidiTimeline& m_timeline;
};
