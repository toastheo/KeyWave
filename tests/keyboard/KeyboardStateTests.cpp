#include <catch2/catch_test_macros.hpp>
#include <vector>

#include "keyboard/KeyboardState.hpp"

namespace {

TEST_CASE("KeyboardState reports active pitches and merges duplicates", "[keyboard][state]")
{
  const KeyboardState state(std::vector{
    Note{.pitch = 60, .velocity = 72},
    Note{.pitch = 60, .velocity = 105},
    Note{.pitch = 64, .velocity = 90},
  });

  CHECK(state.isActive(60));
  CHECK(state.isActive(64));
  CHECK_FALSE(state.isActive(61));
}

} // namespace
