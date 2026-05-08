#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include "keyboard/KeyboardLayout.hpp"

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
  CHECK(layout.height == Catch::Approx(1.0));
  CHECK_FALSE(layout.empty());

  CHECK(layout.whiteKeys[0].pitch == 60);
  CHECK(layout.whiteKeys[0].kind == PianoKeyKind::White);
  CHECK(layout.whiteKeys[0].rect.y == Catch::Approx(-1.0));
  CHECK(layout.whiteKeys[0].rect.height == Catch::Approx(1.0));

  CHECK(layout.blackKeys[0].pitch == 61);
  CHECK(layout.blackKeys[0].kind == PianoKeyKind::Black);
  CHECK(layout.blackKeys[0].rect.y == Catch::Approx(-0.62));
  CHECK(layout.blackKeys[0].rect.height == Catch::Approx(0.62));
}

} // namespace
