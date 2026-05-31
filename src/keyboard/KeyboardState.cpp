#include "keyboard/KeyboardState.hpp"

#include <algorithm>
#include <utility>

KeyboardState::KeyboardState(std::vector<ActiveKey> activeKeys)
    : m_activeKeys(std::move(activeKeys))
{
  m_maxVelocityByPitch.reserve(m_activeKeys.size());

  for (const auto& key : m_activeKeys) {
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

const std::vector<ActiveKey>& KeyboardState::keys() const
{
  return m_activeKeys;
}

bool KeyboardState::empty() const
{
  return m_activeKeys.empty();
}

KeyboardState KeyboardStateBuilder::build(const std::vector<Note>& activeNotes)
{
  std::vector<ActiveKey> activeKeys;
  activeKeys.reserve(activeNotes.size());

  for (const auto& note : activeNotes) {
    activeKeys.push_back(ActiveKey{
      .pitch = note.pitch,
      .velocity = note.velocity,
      .channel = note.channel,
      .track = note.track,
    });
  }

  return KeyboardState(std::move(activeKeys));
}
