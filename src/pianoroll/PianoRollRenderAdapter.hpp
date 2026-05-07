#pragma once

#include <vector>

#include "pianoroll/PianoRollTypes.hpp"
#include "render/RenderCommand.hpp"
#include "render/RenderTypes.hpp"

struct PianoRollRenderStyle
{
  Color noteColor{.r = 0.2f, .g = 0.7f, .b = 1.0f, .a = 1.0f};
  Color clippedNoteColor{.r = 1.0f, .g = 0.6f, .b = 0.2f, .a = 1.0f};
  Color backgroundColor{.r = 0.05f, .g = 0.05f, .b = 0.07f, .a = 1.0f};
  bool includeBackground = false;
};

class PianoRollRenderAdapter
{
public:
  [[nodiscard]] static std::vector<RenderCommand> buildCommands(
    const PianoRollLayoutResult& layout, const PianoRollRenderStyle& style = {});
};
