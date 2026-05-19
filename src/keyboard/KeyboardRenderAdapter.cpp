#include "keyboard/KeyboardRenderAdapter.hpp"

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
  commands.reserve(layout.whiteKeys.size() + layout.blackKeys.size() + layout.whiteKeys.size() + 1);

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

KeyboardRenderStyle keyboardRenderStyleFromSettings(const KeyboardSettings& settings)
{
  const auto sanitizedSettings = sanitizeKeyboardSettings(settings);
  return KeyboardRenderStyle{
    .whiteKeyColor = sanitizedSettings.whiteKeyColor,
    .blackKeyColor = sanitizedSettings.blackKeyColor,
    .activeWhiteKeyColor = sanitizedSettings.activeWhiteKeyColor,
    .activeBlackKeyColor = sanitizedSettings.activeBlackKeyColor,
    .whiteKeySeparatorColor = sanitizedSettings.whiteKeySeparatorColor,
    .hitLineColor = sanitizedSettings.hitLineColor,
    .separatorThicknessPixels = sanitizedSettings.separatorWidth,
    .hitLineHeight = sanitizedSettings.hitLineHeight,
    .includeSeparators = sanitizedSettings.includeSeparators,
    .includeHitLine = sanitizedSettings.includeHitLine,
  };
}
