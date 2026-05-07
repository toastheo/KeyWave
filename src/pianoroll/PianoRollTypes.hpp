#pragma once

#include <vector>

#include "midi/MidiTimelineQuery.hpp"
#include "midi/MidiTypes.hpp"

struct PianoRollLayoutConfig
{
  double noteHeight = 1.0;
  double noteVerticalGap = 0.05;
};

struct PianoRollLayoutViewport
{
  TimeRange timeRange;
  PitchRange pitchRange;
};

struct PianoRollNoteLayout
{
  Note note;

  double x = 0.0;
  double y = 0.0;
  double width = 0.0;
  double height = 0.0;

  double visibleStartSeconds = 0.0;
  double visibleEndSeconds = 0.0;

  bool clippedLeft = false;
  bool clippedRight = false;
};

struct PianoRollLayoutResult
{
  std::vector<PianoRollNoteLayout> notes;

  TimeRange timeRange;
  PitchRange pitchRange;

  int pitchLaneCount = 0;
  double contentWidth = 0.0;
  double contentHeight = 0.0;

  [[nodiscard]] bool empty() const;
};
