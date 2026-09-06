#pragma once

#include <span>
#include <unordered_set>

#include "midi/MidiTypes.hpp"

struct KeyboardState
{
  KeyboardState() = default;
  explicit KeyboardState(std::span<const Note> activeNotes);

  [[nodiscard]] bool isActive(int pitch) const;

private:
  std::unordered_set<int> m_activePitches;
};
