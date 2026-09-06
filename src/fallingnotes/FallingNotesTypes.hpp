#pragma once

#include <vector>

#include "core/CoreTypes.hpp"

struct FallingNotesLayoutStyle
{
  double noteHorizontalInset = 0.0;
  double blackNoteWidthScale = 1.0;
  double whiteNoteWidthScale = 1.0;
};

struct FallingNotesViewport
{
  PitchRange pitchRange;
  double currentTimeSeconds = 0.0;
  double lookAheadSeconds = 10.0;
  double visiblePastSeconds = 0.0;
  double displayHeight = 0.0;
};

struct FallingNoteLayout
{
  double x = 0.0;
  double y = 0.0;
  double width = 0.0;
  double height = 0.0;
};

struct FallingNotesLayoutResult
{
  std::vector<FallingNoteLayout> notes;
};
