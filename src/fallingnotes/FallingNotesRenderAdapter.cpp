#include "fallingnotes/FallingNotesRenderAdapter.hpp"

#include <cmath>

namespace {

constexpr float kBottomGradientScale = 0.7f;

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

Color bottomGradientColorFor(const Color color)
{
  return Color{
    .r = color.r * kBottomGradientScale,
    .g = color.g * kBottomGradientScale,
    .b = color.b * kBottomGradientScale,
    .a = color.a,
  };
}

double borderThicknessFor(const FallingNotesRenderStyle& style)
{
  if (!style.includeOutline) {
    return 0.0;
  }

  return positiveOrZero(style.outlineThicknessPixels);
}

CornerRadiiPixels cornerRadiiForNote(const FallingNoteLayout&, const FallingNotesRenderStyle& style)
{
  const auto radius = positiveOrZero(style.cornerRadiusPixels);
  return CornerRadiiPixels{
    .topLeft = radius,
    .topRight = radius,
    .bottomRight = radius,
    .bottomLeft = radius,
  };
}

void appendStyledRect(std::vector<RenderCommand>& commands,
                      const FallingNoteLayout& noteLayout,
                      const Color& topColor,
                      const FallingNotesRenderStyle& style)
{
  const auto rect = rectForNote(noteLayout);
  if (!isValidRect(rect)) {
    return;
  }

  commands.emplace_back(DrawStyledRectCommand{
    .rect = rect,
    .topColor = topColor,
    .bottomColor = bottomGradientColorFor(topColor),
    .borderColor = style.outlineColor,
    .borderThicknessPixels = borderThicknessFor(style),
    .cornerRadiiPixels = cornerRadiiForNote(noteLayout, style),
  });
}

} // namespace

std::vector<RenderCommand> FallingNotesRenderAdapter::buildCommands(
  const FallingNotesLayoutResult& layout, const FallingNotesRenderStyle& style)
{
  std::vector<RenderCommand> commands;
  commands.reserve(layout.notes.size());

  for (const auto& noteLayout : layout.notes) {
    const auto rect = rectForNote(noteLayout);
    if (!isValidRect(rect)) {
      continue;
    }

    appendStyledRect(commands, noteLayout, colorForNote(noteLayout, style), style);
  }

  return commands;
}
