#pragma once

#include <vector>

#include "core/CoreTypes.hpp"
#include "diagnostics/Diagnostics.hpp"
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
  explicit MidiTimelineQuery(const MidiTimeline& timeline,
                             DiagnosticSink& diagnostics = nullDiagnosticSink());

  [[nodiscard]] std::vector<QueriedNote> findNotes(const TimelineViewport& viewport) const;
  [[nodiscard]] std::vector<Note> findActiveNotesAt(double timeSeconds) const;

private:
  const MidiTimeline& m_timeline;
  DiagnosticSink& m_diagnostics;
};
