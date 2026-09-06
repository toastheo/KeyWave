#pragma once

#include <variant>

#include "render/RenderTypes.hpp"

struct DrawRectCommand
{
  Rect rect;
  Color color;
};

struct DrawTriangleCommand
{
  Vec2 a;
  Vec2 b;
  Vec2 c;
  Color color;
};

struct CornerRadiiPixels
{
  double topLeft = 0.0;
  double topRight = 0.0;
  double bottomRight = 0.0;
  double bottomLeft = 0.0;
};

struct DrawStyledRectCommand
{
  Rect rect;
  Color topColor;
  Color bottomColor;
  Color borderColor;
  double borderThicknessPixels = 0.0;
  CornerRadiiPixels cornerRadiiPixels;
};

struct DrawLineCommand
{
  Vec2 from;
  Vec2 to;
  Color color;
  double thickness = 1.0;
};

using RenderCommand =
  std::variant<DrawRectCommand, DrawStyledRectCommand, DrawLineCommand, DrawTriangleCommand>;
