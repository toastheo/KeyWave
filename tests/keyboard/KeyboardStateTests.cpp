#include <catch2/catch_test_macros.hpp>

#include "keyboard/KeyboardState.hpp"

namespace {

TEST_CASE("KeyboardState reports active pitches and maximum velocity", "[keyboard][state]")
{
  const KeyboardState state(std::vector{
    ActiveKey{.pitch = 60, .velocity = 72},
    ActiveKey{.pitch = 60, .velocity = 105},
    ActiveKey{.pitch = 64, .velocity = 90},
  });

  CHECK(state.isActive(60));
  CHECK(state.isActive(64));
  CHECK_FALSE(state.isActive(61));
  CHECK(state.velocityForPitch(60) == 105);
  CHECK(state.velocityForPitch(61) == 0);
}

} // namespace
