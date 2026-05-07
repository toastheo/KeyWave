#pragma once

#include "render/RenderTypes.hpp"

struct WorldRect
{
  double x = 0.0;
  double y = 0.0;
  double width = 0.0;
  double height = 0.0;
};

inline constexpr WorldRect kDefaultVisibleWorldRect{
  .x = 0.0,
  .y = 0.0,
  .width = 10.0,
  .height = 88.0,
};

struct RendererView
{
  WorldRect visibleWorldRect = kDefaultVisibleWorldRect;
};

[[nodiscard]] bool isValid(const WorldRect& rect);
[[nodiscard]] Vec2 worldToClip(const Vec2& world, const WorldRect& view);
