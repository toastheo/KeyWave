#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include "render/RendererView.hpp"

TEST_CASE("RendererView provides a safe default visible world rectangle", "[render][view]")
{
  constexpr RendererView view;

  CHECK(view.visibleWorldRect.x == Catch::Approx(0.0));
  CHECK(view.visibleWorldRect.y == Catch::Approx(0.0));
  CHECK(view.visibleWorldRect.width == Catch::Approx(10.0));
  CHECK(view.visibleWorldRect.height == Catch::Approx(88.0));
  CHECK(isValid(view.visibleWorldRect));
}

TEST_CASE("WorldRect validation rejects non-positive dimensions", "[render][view]")
{
  CHECK(isValid(WorldRect{.x = -2.0, .y = 4.0, .width = 6.0, .height = 8.0}));
  CHECK_FALSE(isValid(WorldRect{.x = 0.0, .y = 0.0, .width = 0.0, .height = 8.0}));
  CHECK_FALSE(isValid(WorldRect{.x = 0.0, .y = 0.0, .width = 6.0, .height = -1.0}));
}

TEST_CASE("worldToClip maps a bottom-left world rectangle into OpenGL clip space", "[render][view]")
{
  constexpr WorldRect view{.x = 10.0, .y = 20.0, .width = 5.0, .height = 10.0};

  const auto bottomLeft = worldToClip(Vec2{.x = 10.0, .y = 20.0}, view);
  CHECK(bottomLeft.x == Catch::Approx(-1.0));
  CHECK(bottomLeft.y == Catch::Approx(-1.0));

  const auto center = worldToClip(Vec2{.x = 12.5, .y = 25.0}, view);
  CHECK(center.x == Catch::Approx(0.0));
  CHECK(center.y == Catch::Approx(0.0));

  const auto topRight = worldToClip(Vec2{.x = 15.0, .y = 30.0}, view);
  CHECK(topRight.x == Catch::Approx(1.0));
  CHECK(topRight.y == Catch::Approx(1.0));
}
