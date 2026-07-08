#include "keyboard/KeyboardState.hpp"

#include <algorithm>
#include <span>
#include <utility>
#include <vector>

#include "midi/MidiTypes.hpp"

KeyboardState::KeyboardState(const std::vector<ActiveKey>& activeKeys)
{
  m_maxVelocityByPitch.reserve(activeKeys.size());

  for (const auto& key : activeKeys) {
    auto& maxVelocity = m_maxVelocityByPitch[key.pitch];
    maxVelocity = std::max(maxVelocity, key.velocity);
  }
}

bool KeyboardState::isActive(const int pitch) const
{
  return m_maxVelocityByPitch.contains(pitch);
}

int KeyboardState::velocityForPitch(const int pitch) const
{
  const auto velocity = m_maxVelocityByPitch.find(pitch);
  if (velocity == m_maxVelocityByPitch.end()) {
    return 0;
  }

  return velocity->second;
}

KeyboardState KeyboardStateBuilder::build(const std::span<const Note> activeNotes)
{
  std::vector<ActiveKey> activeKeys;
  activeKeys.reserve(activeNotes.size());

  for (const auto& note : activeNotes) {
    activeKeys.push_back(ActiveKey{
      .pitch = note.pitch,
      .velocity = note.velocity,
    });
  }

  return KeyboardState(std::move(activeKeys));
}
