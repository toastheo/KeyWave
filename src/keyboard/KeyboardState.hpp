#pragma once

#include <vector>

#include "midi/MidiTypes.hpp"

struct ActiveKey
{
  int pitch = 0;
  int velocity = 0;
  int channel = 0;
  int track = 0;
};

struct KeyboardState
{
  std::vector<ActiveKey> activeKeys;

  [[nodiscard]] bool isActive(int pitch) const;
  [[nodiscard]] int velocityForPitch(int pitch) const;
  [[nodiscard]] bool empty() const;
};

class KeyboardStateBuilder
{
public:
  [[nodiscard]] static KeyboardState build(const std::vector<Note>& activeNotes);
};
