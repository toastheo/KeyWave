#pragma once

#include <cstdint>
#include <vector>

#include "midi/MidiTypes.hpp"
#include "render/RenderTypes.hpp"

enum class PianoKeyKind : std::uint8_t
{
  White,
  Black,
};

struct PianoKeyLayout
{
  int pitch = 0;
  PianoKeyKind kind = PianoKeyKind::White;
  Rect rect;
  bool active = false;
  int velocity = 0;
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

  PitchRange pitchRange;
  double width = 0.0;
  double height = 0.0;

  [[nodiscard]] bool empty() const;
};
