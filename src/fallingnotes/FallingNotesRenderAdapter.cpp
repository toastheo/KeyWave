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

  if (noteLayout.clippedBottom || noteLayout.clippedTop) {
    return style.clippedNoteColor;
  }

  return style.noteColor;
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

    commands.emplace_back(DrawRectCommand{
      .rect = rect,
      .color = colorForNote(noteLayout, style),
    });
  }

  return commands;
}

FallingNotesRenderStyle fallingNotesRenderStyleFromSettings(const FallingNotesSettings& settings)
{
  const auto sanitizedSettings = sanitizeFallingNotesSettings(settings);
  return FallingNotesRenderStyle{
    .noteColor = sanitizedSettings.noteColor,
    .activeNoteColor = sanitizedSettings.activeNoteColor,
    .clippedNoteColor = sanitizedSettings.clippedNoteColor,
  };
}
