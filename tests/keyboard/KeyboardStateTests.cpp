#include <catch2/catch_test_macros.hpp>

#include "keyboard/KeyboardState.hpp"

namespace {

TEST_CASE("KeyboardState reports active pitches and maximum velocity", "[keyboard][state]")
{
  const KeyboardState state(std::vector{
    ActiveKey{.pitch = 60, .velocity = 72, .channel = 0, .track = 0},
    ActiveKey{.pitch = 60, .velocity = 105, .channel = 1, .track = 0},
    ActiveKey{.pitch = 64, .velocity = 90, .channel = 0, .track = 1},
  });

  CHECK(state.isActive(60));
  CHECK(state.isActive(64));
  CHECK_FALSE(state.isActive(61));
  CHECK(state.velocityForPitch(60) == 105);
  CHECK(state.velocityForPitch(61) == 0);
  CHECK_FALSE(state.empty());
}

TEST_CASE("KeyboardStateBuilder preserves active note identity", "[keyboard][state]")
{
  const std::vector activeNotes{
    Note{.pitch = 60,
         .velocity = 72,
         .channel = 0,
         .track = 2,
         .startSeconds = 1.0,
         .durationSeconds = 2.0},
    Note{.pitch = 64,
         .velocity = 90,
         .channel = 1,
         .track = 3,
         .startSeconds = 1.0,
         .durationSeconds = 2.0},
  };

  const auto state = KeyboardStateBuilder::build(activeNotes);
  const auto& activeKeys = state.keys();

  REQUIRE(activeKeys.size() == 2);
  CHECK(activeKeys[0].pitch == 60);
  CHECK(activeKeys[0].velocity == 72);
  CHECK(activeKeys[0].channel == 0);
  CHECK(activeKeys[0].track == 2);
  CHECK(activeKeys[1].pitch == 64);
  CHECK(activeKeys[1].velocity == 90);
  CHECK(activeKeys[1].channel == 1);
  CHECK(activeKeys[1].track == 3);
}

} // namespace
