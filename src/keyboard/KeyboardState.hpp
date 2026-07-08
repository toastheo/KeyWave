#pragma once

#include <span>
#include <unordered_map>
#include <vector>

#include "midi/MidiTypes.hpp"

struct ActiveKey
{
  int pitch = 0;
  int velocity = 0;
};

struct KeyboardState
{
  KeyboardState() = default;
  explicit KeyboardState(const std::vector<ActiveKey>& activeKeys);

  [[nodiscard]] bool isActive(int pitch) const;
  [[nodiscard]] int velocityForPitch(int pitch) const;

private:
  std::unordered_map<int, int> m_maxVelocityByPitch;
};

class KeyboardStateBuilder
{
public:
  [[nodiscard]] static KeyboardState build(std::span<const Note> activeNotes);
};
