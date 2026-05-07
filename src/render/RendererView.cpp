#include "render/RendererView.hpp"

#include <cmath>

bool isValid(const WorldRect& rect)
{
  return std::isfinite(rect.x) && std::isfinite(rect.y) && std::isfinite(rect.width) &&
         std::isfinite(rect.height) && rect.width > 0.0 && rect.height > 0.0;
}

Vec2 worldToClip(const Vec2& world, const WorldRect& view)
{
  return Vec2{
    .x = ((world.x - view.x) / view.width * 2.0) - 1.0,
    .y = ((world.y - view.y) / view.height * 2.0) - 1.0,
  };
}
