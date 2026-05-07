#include "pianoroll/PianoRollRenderAdapter.hpp"

namespace {

bool isValidRect(const Rect& rect)
{
  return rect.width > 0.0 && rect.height > 0.0;
}

Rect rectForNote(const PianoRollNoteLayout& noteLayout)
{
  return Rect{
    .x = noteLayout.x,
    .y = noteLayout.y,
    .width = noteLayout.width,
    .height = noteLayout.height,
  };
}

Color colorForNote(const PianoRollNoteLayout& noteLayout, const PianoRollRenderStyle& style)
{
  return noteLayout.clippedLeft || noteLayout.clippedRight ? style.clippedNoteColor
                                                           : style.noteColor;
}

} // namespace

std::vector<RenderCommand> PianoRollRenderAdapter::buildCommands(
  const PianoRollLayoutResult& layout, const PianoRollRenderStyle& style)
{
  std::vector<RenderCommand> commands;
  commands.reserve(layout.notes.size() + (style.includeBackground ? 1U : 0U));

  if (style.includeBackground) {
    const Rect backgroundRect{
      .x = 0.0,
      .y = 0.0,
      .width = layout.contentWidth,
      .height = layout.contentHeight,
    };
    if (isValidRect(backgroundRect)) {
      commands.emplace_back(DrawRectCommand{
        .rect = backgroundRect,
        .color = style.backgroundColor,
      });
    }
  }

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
