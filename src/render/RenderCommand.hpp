#pragma once

#include <variant>

#include "render/RenderTypes.hpp"

struct DrawRectCommand
{
  Rect rect;
  Color color;
};

struct DrawLineCommand
{
  Vec2 from;
  Vec2 to;
  Color color;
  double thickness = 1.0;
};

using RenderCommand = std::variant<DrawRectCommand, DrawLineCommand>;
