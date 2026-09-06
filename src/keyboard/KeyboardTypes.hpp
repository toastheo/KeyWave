#pragma once

#include <vector>

#include "core/CoreTypes.hpp"

struct PianoKeyLayout
{
  Rect rect;
  bool active = false;
};

struct KeyboardLayoutConfig
{
  PitchRange pitchRange{.minPitch = 21, .maxPitch = 108};

  double whiteKeyWidth = 1.0;
  double whiteKeyHeight = 2.5;

  double blackKeyWidth = 0.6;
  double blackKeyHeight = 1.55;

  double whiteKeyGap = 0.015;
};

struct KeyboardLayoutResult
{
  std::vector<PianoKeyLayout> whiteKeys;
  std::vector<PianoKeyLayout> blackKeys;

  double width = 0.0;
  double height = 0.0;
};
