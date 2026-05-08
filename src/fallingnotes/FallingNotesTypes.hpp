#pragma once

#include <vector>

#include "midi/MidiTimelineQuery.hpp"
#include "midi/MidiTypes.hpp"

struct FallingNotesViewport
{
  PitchRange pitchRange;
  double currentTimeSeconds = 0.0;
  double lookAheadSeconds = 10.0;
  double visiblePastSeconds = 0.0;
};

struct FallingNotesLayoutConfig
{
  double laneWidth = 1.0;
  double noteHorizontalGap = 0.08;
};

struct FallingNoteLayout
{
  Note note;

  double x = 0.0;
  double y = 0.0;
  double width = 0.0;
  double height = 0.0;

  double visibleStartOffsetSeconds = 0.0;
  double visibleEndOffsetSeconds = 0.0;

  bool clippedBottom = false;
  bool clippedTop = false;
};

struct FallingNotesLayoutResult
{
  std::vector<FallingNoteLayout> notes;

  PitchRange pitchRange;

  double currentTimeSeconds = 0.0;
  double lookAheadSeconds = 0.0;
  double visiblePastSeconds = 0.0;

  int pitchLaneCount = 0;
  double contentWidth = 0.0;
  double contentHeight = 0.0;

  [[nodiscard]] bool empty() const;
};
