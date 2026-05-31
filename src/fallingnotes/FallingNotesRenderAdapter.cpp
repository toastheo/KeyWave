#include "fallingnotes/FallingNotesRenderAdapter.hpp"

#include <cmath>

namespace {

bool isValidRect(const Rect& rect)
{
  return std::isfinite(rect.x) && std::isfinite(rect.y) && std::isfinite(rect.width) &&
         std::isfinite(rect.height) && rect.width > 0.0 && rect.height > 0.0;
}

Rect rectForNote(const FallingNoteLayout& noteLayout)
{
  return Rect{
    .x = noteLayout.x,
    .y = noteLayout.y,
    .width = noteLayout.width,
    .height = noteLayout.height,
  };
}

bool intersectsHitLine(const FallingNoteLayout& noteLayout)
{
  return noteLayout.y <= 0.0 && noteLayout.y + noteLayout.height >= 0.0;
}

Color colorForNote(const FallingNoteLayout& noteLayout, const FallingNotesRenderStyle& style)
{
  if (intersectsHitLine(noteLayout)) {
    return style.activeNoteColor;
  }

  return style.noteColor;
}

double positiveOrZero(const double value)
{
  return std::isfinite(value) && value > 0.0 ? value : 0.0;
}

void appendRect(std::vector<RenderCommand>& commands, const Rect& rect, const Color& color)
{
  if (!isValidRect(rect)) {
    return;
  }

  commands.emplace_back(DrawRectCommand{
    .rect = rect,
    .color = color,
  });
}

void appendOutline(std::vector<RenderCommand>& commands,
                   const Rect& rect,
                   const FallingNotesRenderStyle& style)
{
  if (!style.includeOutline) {
    return;
  }

  const auto thickness = positiveOrZero(style.outlineThicknessPixels);
  if (thickness <= 0.0) {
    return;
  }

  const auto left = rect.x;
  const auto right = rect.x + rect.width;
  const auto bottom = rect.y;
  const auto top = rect.y + rect.height;

  commands.emplace_back(DrawLineCommand{
    .from = Vec2{.x = left, .y = bottom},
    .to = Vec2{.x = left, .y = top},
    .color = style.outlineColor,
    .thickness = thickness,
  });
  commands.emplace_back(DrawLineCommand{
    .from = Vec2{.x = right, .y = bottom},
    .to = Vec2{.x = right, .y = top},
    .color = style.outlineColor,
    .thickness = thickness,
  });
  commands.emplace_back(DrawLineCommand{
    .from = Vec2{.x = left, .y = bottom},
    .to = Vec2{.x = right, .y = bottom},
    .color = style.outlineColor,
    .thickness = thickness,
  });
  commands.emplace_back(DrawLineCommand{
    .from = Vec2{.x = left, .y = top},
    .to = Vec2{.x = right, .y = top},
    .color = style.outlineColor,
    .thickness = thickness,
  });
}

} // namespace

std::vector<RenderCommand> FallingNotesRenderAdapter::buildCommands(
  const FallingNotesLayoutResult& layout, const FallingNotesRenderStyle& style)
{
  std::vector<RenderCommand> commands;
  commands.reserve(layout.notes.size() * (style.includeOutline ? 5U : 1U));

  for (const auto& noteLayout : layout.notes) {
    const auto rect = rectForNote(noteLayout);
    if (!isValidRect(rect)) {
      continue;
    }

    appendRect(commands, rect, colorForNote(noteLayout, style));
    appendOutline(commands, rect, style);
  }

  return commands;
}
