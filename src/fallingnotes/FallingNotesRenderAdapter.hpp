#pragma once

#include <vector>

#include "app/AppSettings.hpp"
#include "fallingnotes/FallingNotesTypes.hpp"
#include "render/RenderCommand.hpp"
#include "render/RenderTypes.hpp"

struct FallingNotesRenderStyle
{
  Color noteColor{.r = 0.2f, .g = 0.7f, .b = 1.0f, .a = 1.0f};
  Color activeNoteColor{.r = 0.4f, .g = 1.0f, .b = 0.5f, .a = 1.0f};
};

[[nodiscard]] FallingNotesRenderStyle fallingNotesRenderStyleFromSettings(
  const FallingNotesSettings& settings);

class FallingNotesRenderAdapter
{
public:
  [[nodiscard]] static std::vector<RenderCommand> buildCommands(
    const FallingNotesLayoutResult& layout, const FallingNotesRenderStyle& style = {});
};
