#include "keyboard/KeyboardRenderAdapter.hpp"

#include <algorithm>
#include <cmath>

namespace {

bool isValidRect(const Rect& rect)
{
  return std::isfinite(rect.x) && std::isfinite(rect.y) && std::isfinite(rect.width) &&
         std::isfinite(rect.height) && rect.width > 0.0 && rect.height > 0.0;
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

} // namespace

std::vector<RenderCommand> KeyboardRenderAdapter::buildCommands(
  const KeyboardLayoutResult& layout, const KeyboardRenderStyle& style)
{
  std::vector<RenderCommand> commands;
  commands.reserve(layout.whiteKeys.size() + layout.blackKeys.size() + layout.whiteKeys.size() + 1);

  for (const auto& key : layout.whiteKeys) {
    appendRect(commands, key.rect, style.whiteKeyColor);
  }

  if (style.includeSeparators && layout.whiteKeys.size() > 1) {
    const auto separatorWidth = positiveOrZero(style.separatorWidth);
    if (separatorWidth > 0.0) {
      for (auto index = std::size_t{0}; index + 1 < layout.whiteKeys.size(); ++index) {
        const auto& key = layout.whiteKeys[index];
        appendRect(commands,
                   Rect{
                     .x = key.rect.x + key.rect.width - (separatorWidth * 0.5),
                     .y = -layout.height,
                     .width = separatorWidth,
                     .height = layout.height,
                   },
                   style.whiteKeySeparatorColor);
      }
    }
  }

  for (const auto& key : layout.blackKeys) {
    appendRect(commands, key.rect, style.blackKeyColor);
  }

  if (style.includeHitLine) {
    const auto hitLineHeight = positiveOrZero(style.hitLineHeight);
    appendRect(commands,
               Rect{
                 .x = 0.0,
                 .y = 0.0,
                 .width = layout.width,
                 .height = hitLineHeight,
               },
               style.hitLineColor);
  }

  return commands;
}
