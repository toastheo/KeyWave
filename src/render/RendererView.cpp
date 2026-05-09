#include "render/RendererView.hpp"

#include <algorithm>
#include <cmath>

#include "render/RenderCommand.hpp"

namespace {

constexpr double kAxisEpsilon = 0.000000001;

struct PixelAxisProjection
{
  double worldOrigin = 0.0;
  double worldExtent = 0.0;
  int pixelExtent = 0;
};

bool nearlyEqual(const double left, const double right)
{
  return std::abs(left - right) <= kAxisEpsilon;
}

bool isFinitePoint(const Vec2& point)
{
  return std::isfinite(point.x) && std::isfinite(point.y);
}

int positivePixelThickness(const double thickness)
{
  if (!std::isfinite(thickness) || thickness <= 0.0) {
    return 0;
  }

  return std::max(1, static_cast<int>(std::round(thickness)));
}

// Convert a center line in world coordinates into pixel-aligned rectangle edge.
// This keeps 1px separators crisp even when the world-to-framebuffer scale is fractional.
double pixelAlignedWorldStart(const double worldPosition,
                              const PixelAxisProjection& axis,
                              const int thicknessPixels)
{
  const auto pixelPosition =
    (worldPosition - axis.worldOrigin) / axis.worldExtent * axis.pixelExtent;
  const auto startPixel = std::round(pixelPosition - (static_cast<double>(thicknessPixels) * 0.5));
  return axis.worldOrigin + (startPixel / static_cast<double>(axis.pixelExtent) * axis.worldExtent);
}

double pixelThicknessToWorld(const int thicknessPixels, const PixelAxisProjection& axis)
{
  return static_cast<double>(thicknessPixels) / static_cast<double>(axis.pixelExtent) *
         axis.worldExtent;
}

} // namespace

bool isValid(const WorldRect& rect)
{
  return std::isfinite(rect.x) && std::isfinite(rect.y) && std::isfinite(rect.width) &&
         std::isfinite(rect.height) && rect.width > 0.0 && rect.height > 0.0;
}

bool isValid(const FramebufferSize& size)
{
  return size.width > 0 && size.height > 0;
}

Vec2 worldToClip(const Vec2& world, const WorldRect& view)
{
  return Vec2{
    .x = ((world.x - view.x) / view.width * 2.0) - 1.0,
    .y = ((world.y - view.y) / view.height * 2.0) - 1.0,
  };
}

Rect lineToPixelAlignedRect(const DrawLineCommand& line,
                            const RendererView& view,
                            const FramebufferSize& framebufferSize)
{
  if (!isFinitePoint(line.from) || !isFinitePoint(line.to) || !isValid(view.visibleWorldRect) ||
      !isValid(framebufferSize)) {
    return {};
  }

  const auto thicknessPixels = positivePixelThickness(line.thickness);
  if (thicknessPixels == 0) {
    return {};
  }

  if (nearlyEqual(line.from.x, line.to.x)) {
    const auto height = std::abs(line.to.y - line.from.y);
    if (height <= 0.0) {
      return {};
    }

    const PixelAxisProjection xAxis{
      .worldOrigin = view.visibleWorldRect.x,
      .worldExtent = view.visibleWorldRect.width,
      .pixelExtent = framebufferSize.width,
    };

    return Rect{
      .x = pixelAlignedWorldStart(line.from.x, xAxis, thicknessPixels),
      .y = std::min(line.from.y, line.to.y),
      .width = pixelThicknessToWorld(thicknessPixels, xAxis),
      .height = height,
    };
  }

  if (nearlyEqual(line.from.y, line.to.y)) {
    const auto width = std::abs(line.to.x - line.from.x);
    if (width <= 0.0) {
      return {};
    }

    const PixelAxisProjection yAxis{
      .worldOrigin = view.visibleWorldRect.y,
      .worldExtent = view.visibleWorldRect.height,
      .pixelExtent = framebufferSize.height,
    };

    return Rect{
      .x = std::min(line.from.x, line.to.x),
      .y = pixelAlignedWorldStart(line.from.y, yAxis, thicknessPixels),
      .width = width,
      .height = pixelThicknessToWorld(thicknessPixels, yAxis),
    };
  }

  return {};
}
