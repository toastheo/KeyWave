#pragma once

#include <unordered_map>
#include <span>
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
  KeyboardState() = default;
  explicit KeyboardState(std::vector<ActiveKey> activeKeys);

  [[nodiscard]] bool isActive(int pitch) const;
  [[nodiscard]] int velocityForPitch(int pitch) const;
  [[nodiscard]] const std::vector<ActiveKey>& keys() const;
  [[nodiscard]] bool empty() const;

private:
  std::vector<ActiveKey> m_activeKeys;
  std::unordered_map<int, int> m_maxVelocityByPitch;
};

class KeyboardStateBuilder
{
public:
  [[nodiscard]] static KeyboardState build(std::span<const Note> activeNotes);
};
