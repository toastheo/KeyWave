#pragma once

#include <cstdint>
#include <variant>

#include "render/RenderTypes.hpp"

struct DrawRectCommand
{
  Rect rect;
  Color color;
};

struct DrawStyledRectCommand
{
  Rect rect;
  Color topColor;
  Color bottomColor;
  Color borderColor;
  double borderThicknessPixels = 0.0;
};

// Butt caps end at the line endpoints. Square caps extend by half the stroke
// thickness, which lets adjacent outline segments overlap cleanly at corners.
enum class LineCap : std::uint8_t
{
  Butt,
  Square,
};

struct DrawLineCommand
{
  Vec2 from;
  Vec2 to;
  Color color;
  double thickness = 1.0;
  LineCap cap = LineCap::Butt;
};

using RenderCommand = std::variant<DrawRectCommand, DrawStyledRectCommand, DrawLineCommand>;
