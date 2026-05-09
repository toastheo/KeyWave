#include "keyboard/KeyboardState.hpp"

#include <algorithm>

bool KeyboardState::isActive(const int pitch) const
{
  return std::ranges::any_of(activeKeys,
                             [pitch](const ActiveKey& key) { return key.pitch == pitch; });
}

int KeyboardState::velocityForPitch(const int pitch) const
{
  auto maxVelocity = 0;
  for (const auto& key : activeKeys) {
    if (key.pitch == pitch) {
      maxVelocity = std::max(maxVelocity, key.velocity);
    }
  }

  return maxVelocity;
}

bool KeyboardState::empty() const
{
  return activeKeys.empty();
}

KeyboardState KeyboardStateBuilder::build(const std::vector<Note>& activeNotes)
{
  KeyboardState state;
  state.activeKeys.reserve(activeNotes.size());

  for (const auto& note : activeNotes) {
    state.activeKeys.push_back(ActiveKey{
      .pitch = note.pitch,
      .velocity = note.velocity,
      .channel = note.channel,
      .track = note.track,
    });
  }

  return state;
}
