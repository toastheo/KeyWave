#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include "keyboard/KeyboardGeometry.hpp"

namespace {

TEST_CASE("KeyboardGeometry builds an 88-key piano range without assuming C starts",
          "[keyboard][geometry]")
{
  const KeyboardGeometry geometry;

  CHECK(geometry.containsPitch(21));
  CHECK(geometry.containsPitch(108));
  CHECK_FALSE(geometry.containsPitch(20));
  CHECK_FALSE(geometry.containsPitch(109));
  CHECK(geometry.whiteKeyCount() == 52);
  CHECK(geometry.width() == Catch::Approx(52.0));
  CHECK(geometry.height() == Catch::Approx(1.0));

  const auto a0 = geometry.keyRectForPitch(21);
  CHECK(a0.x == Catch::Approx(0.0));
  CHECK(a0.y == Catch::Approx(-1.0));
  CHECK(a0.width == Catch::Approx(1.0));
  CHECK(a0.height == Catch::Approx(1.0));

  const auto aSharp0 = geometry.keyRectForPitch(22);
  CHECK(aSharp0.x == Catch::Approx(0.7));
  CHECK(aSharp0.y == Catch::Approx(-0.62));
  CHECK(aSharp0.width == Catch::Approx(0.6));
  CHECK(aSharp0.height == Catch::Approx(0.62));

  const auto c8 = geometry.keyRectForPitch(108);
  CHECK(c8.x == Catch::Approx(51.0));
  CHECK(c8.y == Catch::Approx(-1.0));
}

TEST_CASE("KeyboardGeometry aligns note rectangles with the corresponding key",
          "[keyboard][geometry]")
{
  const KeyboardGeometry geometry(KeyboardLayoutConfig{
    .pitchRange = PitchRange{.minPitch = 60, .maxPitch = 64},
    .whiteKeyWidth = 2.0,
    .whiteKeyHeight = 1.5,
    .blackKeyWidth = 1.0,
    .blackKeyHeight = 0.75,
    .whiteKeyGap = 0.2,
  });

  const auto c4Key = geometry.keyRectForPitch(60);
  const auto c4Note = geometry.noteRectForPitch(60);
  CHECK(c4Key.x == Catch::Approx(0.0));
  CHECK(c4Key.width == Catch::Approx(2.0));
  CHECK(c4Note.x == Catch::Approx(0.1));
  CHECK(c4Note.width == Catch::Approx(1.8));

  const auto cSharp4Key = geometry.keyRectForPitch(61);
  const auto cSharp4Note = geometry.noteRectForPitch(61);
  CHECK(cSharp4Key.x == Catch::Approx(1.5));
  CHECK(cSharp4Key.width == Catch::Approx(1.0));
  CHECK(cSharp4Note.x == Catch::Approx(cSharp4Key.x));
  CHECK(cSharp4Note.width == Catch::Approx(cSharp4Key.width));
}

TEST_CASE("KeyboardGeometry handles partial ranges and missing neighboring black keys safely",
          "[keyboard][geometry]")
{
  const KeyboardGeometry geometry(KeyboardLayoutConfig{
    .pitchRange = PitchRange{.minPitch = 61, .maxPitch = 61},
  });

  CHECK(geometry.whiteKeyCount() == 0);
  CHECK(geometry.width() == Catch::Approx(0.0));
  CHECK(geometry.keyRectForPitch(61).width == Catch::Approx(0.0));
  CHECK(geometry.noteRectForPitch(61).width == Catch::Approx(0.0));
}

} // namespace
