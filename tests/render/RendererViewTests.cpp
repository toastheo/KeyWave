#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include "render/RenderCommand.hpp"
#include "render/RendererView.hpp"

namespace {

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

TEST_CASE("FramebufferSize validation rejects non-positive dimensions", "[render][view]")
{
  CHECK(isValid(FramebufferSize{.width = 1280, .height = 720}));
  CHECK_FALSE(isValid(FramebufferSize{.width = 0, .height = 720}));
  CHECK_FALSE(isValid(FramebufferSize{.width = 1280, .height = 0}));
  CHECK_FALSE(isValid(FramebufferSize{.width = -1, .height = 720}));
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

TEST_CASE("lineToPixelAlignedRect keeps vertical lines at least one framebuffer pixel wide",
          "[render][view]")
{
  constexpr RendererView view{
    .visibleWorldRect = WorldRect{.x = 0.0, .y = -1.0, .width = 52.0, .height = 11.0},
  };
  constexpr FramebufferSize framebufferSize{.width = 1040, .height = 550};
  constexpr DrawLineCommand line{
    .from = Vec2{.x = 1.01, .y = -1.0},
    .to = Vec2{.x = 1.01, .y = 0.0},
    .color = Color{},
    .thickness = 1.0,
  };

  const auto rect = lineToPixelAlignedRect(line, view, framebufferSize);

  CHECK(rect.x == Catch::Approx(1.0));
  CHECK(rect.y == Catch::Approx(-1.0));
  CHECK(rect.width / view.visibleWorldRect.width * framebufferSize.width == Catch::Approx(1.0));
  CHECK(rect.height == Catch::Approx(1.0));
}

TEST_CASE("lineToPixelAlignedRect extends square-capped lines along their length", "[render][view]")
{
  constexpr RendererView view{
    .visibleWorldRect = WorldRect{.x = 0.0, .y = 0.0, .width = 100.0, .height = 50.0},
  };
  constexpr FramebufferSize framebufferSize{.width = 1000, .height = 500};

  constexpr DrawLineCommand horizontal{
    .from = Vec2{.x = 10.0, .y = 20.0},
    .to = Vec2{.x = 30.0, .y = 20.0},
    .color = Color{},
    .thickness = 4.0,
    .cap = LineCap::Square,
  };
  const auto horizontalRect = lineToPixelAlignedRect(horizontal, view, framebufferSize);
  CHECK(horizontalRect.x == Catch::Approx(9.8));
  CHECK(horizontalRect.width == Catch::Approx(20.4));

  constexpr DrawLineCommand vertical{
    .from = Vec2{.x = 10.0, .y = 20.0},
    .to = Vec2{.x = 10.0, .y = 30.0},
    .color = Color{},
    .thickness = 4.0,
    .cap = LineCap::Square,
  };
  const auto verticalRect = lineToPixelAlignedRect(vertical, view, framebufferSize);
  CHECK(verticalRect.y == Catch::Approx(19.8));
  CHECK(verticalRect.height == Catch::Approx(10.4));
}

} // namespace
