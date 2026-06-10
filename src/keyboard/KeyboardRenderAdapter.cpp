#include "keyboard/KeyboardRenderAdapter.hpp"

#include <algorithm>
#include <cmath>

namespace {

constexpr Color kWhiteKeyCutMaskColor{.r = 0.0f, .g = 0.0f, .b = 0.0f, .a = 1.0f};
constexpr double kWhiteKeyCutWidthRatio = 0.12;
constexpr double kWhiteKeyCutHeightRatio = 0.02;

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

void appendTriangle(std::vector<RenderCommand>& commands,
                    const Vec2 a,
                    const Vec2 b,
                    const Vec2 c,
                    const Color& color)
{
  if (!std::isfinite(a.x) || !std::isfinite(a.y) || !std::isfinite(b.x) ||
      !std::isfinite(b.y) || !std::isfinite(c.x) || !std::isfinite(c.y)) {
    return;
  }

  commands.emplace_back(DrawTriangleCommand{
    .a = a,
    .b = b,
    .c = c,
    .color = color,
  });
}

void appendWhiteKeyCutMasks(std::vector<RenderCommand>& commands, const Rect& rect)
{
  if (!isValidRect(rect)) {
    return;
  }

  const auto cutWidth = std::min(rect.width * kWhiteKeyCutWidthRatio, rect.width * 0.45);
  const auto cutHeight = std::min(rect.height * kWhiteKeyCutHeightRatio, rect.height * 0.45);
  if (cutWidth <= 0.0 || cutHeight <= 0.0) {
    return;
  }

  const auto left = rect.x;
  const auto right = rect.x + rect.width;
  const auto bottom = rect.y;
  const auto shoulderY = rect.y + cutHeight;

  appendTriangle(commands,
                 Vec2{.x = left, .y = bottom},
                 Vec2{.x = left, .y = shoulderY},
                 Vec2{.x = left + cutWidth, .y = bottom},
                 kWhiteKeyCutMaskColor);
  appendTriangle(commands,
                 Vec2{.x = right, .y = bottom},
                 Vec2{.x = right, .y = shoulderY},
                 Vec2{.x = right - cutWidth, .y = bottom},
                 kWhiteKeyCutMaskColor);
}

Color colorForWhiteKey(const PianoKeyLayout& key, const KeyboardRenderStyle& style)
{
  return key.active ? style.activeWhiteKeyColor : style.whiteKeyColor;
}

Color colorForBlackKey(const PianoKeyLayout& key, const KeyboardRenderStyle& style)
{
  return key.active ? style.activeBlackKeyColor : style.blackKeyColor;
}

} // namespace

std::vector<RenderCommand> KeyboardRenderAdapter::buildCommands(const KeyboardLayoutResult& layout,
                                                                const KeyboardRenderStyle& style)
{
  std::vector<RenderCommand> commands;
  commands.reserve((layout.whiteKeys.size() * 4U) + layout.blackKeys.size() + 1U);

  for (const auto& key : layout.whiteKeys) {
    appendRect(commands, key.rect, colorForWhiteKey(key, style));
  }

  if (style.includeSeparators && layout.whiteKeys.size() > 1) {
    const auto separatorThicknessPixels = positiveOrZero(style.separatorThicknessPixels);
    if (separatorThicknessPixels > 0.0) {
      for (auto index = std::size_t{0}; index + 1 < layout.whiteKeys.size(); ++index) {
        const auto& key = layout.whiteKeys[index];
        const auto separatorX = key.rect.x + key.rect.width;
        commands.emplace_back(DrawLineCommand{
          .from = Vec2{.x = separatorX, .y = -layout.height},
          .to = Vec2{.x = separatorX, .y = 0.0},
          .color = style.whiteKeySeparatorColor,
          .thickness = separatorThicknessPixels,
        });
      }
    }
  }

  for (const auto& key : layout.whiteKeys) {
    appendWhiteKeyCutMasks(commands, key.rect);
  }

  for (const auto& key : layout.blackKeys) {
    appendRect(commands, key.rect, colorForBlackKey(key, style));
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
