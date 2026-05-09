#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include "keyboard/KeyboardLayout.hpp"
#include "keyboard/KeyboardState.hpp"

namespace {

TEST_CASE("KeyboardLayout separates white and black keys for painter ordering",
          "[keyboard][layout]")
{
  const KeyboardGeometry geometry(KeyboardLayoutConfig{
    .pitchRange = PitchRange{.minPitch = 60, .maxPitch = 64},
  });

  const auto layout = KeyboardLayout::build(geometry);

  REQUIRE(layout.whiteKeys.size() == 3);
  REQUIRE(layout.blackKeys.size() == 2);
  CHECK(layout.pitchRange.minPitch == 60);
  CHECK(layout.pitchRange.maxPitch == 64);
  CHECK(layout.width == Catch::Approx(3.0));
  CHECK(layout.height == Catch::Approx(2.5));
  CHECK_FALSE(layout.empty());

  CHECK(layout.whiteKeys[0].pitch == 60);
  CHECK(layout.whiteKeys[0].kind == PianoKeyKind::White);
  CHECK(layout.whiteKeys[0].rect.y == Catch::Approx(-2.5));
  CHECK(layout.whiteKeys[0].rect.height == Catch::Approx(2.5));

  CHECK(layout.blackKeys[0].pitch == 61);
  CHECK(layout.blackKeys[0].kind == PianoKeyKind::Black);
  CHECK(layout.blackKeys[0].rect.y == Catch::Approx(-1.55));
  CHECK(layout.blackKeys[0].rect.height == Catch::Approx(1.55));
}

TEST_CASE("KeyboardLayout marks keys active from keyboard state", "[keyboard][layout]")
{
  const KeyboardGeometry geometry(KeyboardLayoutConfig{
    .pitchRange = PitchRange{.minPitch = 60, .maxPitch = 64},
  });
  const KeyboardState state{
    .activeKeys =
      {
        ActiveKey{.pitch = 60, .velocity = 64, .channel = 0, .track = 0},
        ActiveKey{.pitch = 61, .velocity = 100, .channel = 1, .track = 0},
      },
  };

  const auto layout = KeyboardLayout::build(geometry, state);

  REQUIRE(layout.whiteKeys.size() == 3);
  REQUIRE(layout.blackKeys.size() == 2);
  CHECK(layout.whiteKeys[0].pitch == 60);
  CHECK(layout.whiteKeys[0].active);
  CHECK(layout.whiteKeys[0].velocity == 64);
  CHECK_FALSE(layout.whiteKeys[1].active);
  CHECK(layout.whiteKeys[1].velocity == 0);
  CHECK(layout.blackKeys[0].pitch == 61);
  CHECK(layout.blackKeys[0].active);
  CHECK(layout.blackKeys[0].velocity == 100);
}

} // namespace
