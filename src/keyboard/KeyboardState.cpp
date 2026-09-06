#include "keyboard/KeyboardState.hpp"

#include <span>

#include "midi/MidiTypes.hpp"

KeyboardState::KeyboardState(const std::span<const Note> activeNotes)
{
  m_activePitches.reserve(activeNotes.size());
  for (const auto& note : activeNotes) {
    m_activePitches.insert(note.pitch);
  }
}

bool KeyboardState::isActive(const int pitch) const
{
  return m_activePitches.contains(pitch);
}
