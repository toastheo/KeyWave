#pragma once

#include <vector>

#include "fallingnotes/FallingNotesTypes.hpp"
#include "render/RenderCommand.hpp"

struct FallingNotesRenderStyle
{
  Color noteColor{.r = 0.2f, .g = 0.7f, .b = 1.0f, .a = 1.0f};
  Color activeNoteColor{.r = 0.4f, .g = 1.0f, .b = 0.5f, .a = 1.0f};
  Color outlineColor{.r = 0.02f, .g = 0.04f, .b = 0.05f, .a = 1.0f};
  double outlineThicknessPixels = 1.0;
  double cornerRadiusPixels = 4.0;
  bool includeOutline = true;
};

class FallingNotesRenderAdapter
{
public:
  [[nodiscard]] static std::vector<RenderCommand> buildCommands(
    const FallingNotesLayoutResult& layout, const FallingNotesRenderStyle& style = {});
};
