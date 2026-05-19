#pragma once

#include <vector>

#include "app/AppSettings.hpp"
#include "keyboard/KeyboardTypes.hpp"
#include "render/RenderCommand.hpp"

struct KeyboardRenderStyle
{
  Color whiteKeyColor{.r = 0.92f, .g = 0.92f, .b = 0.90f, .a = 1.0f};
  Color blackKeyColor{.r = 0.04f, .g = 0.04f, .b = 0.05f, .a = 1.0f};
  Color activeWhiteKeyColor{.r = 0.4f, .g = 1.0f, .b = 0.5f, .a = 1.0f};
  Color activeBlackKeyColor{.r = 0.2f, .g = 0.68f, .b = 0.28f, .a = 1.0f};
  Color whiteKeySeparatorColor{.r = 0.25f, .g = 0.25f, .b = 0.27f, .a = 1.0f};
  Color hitLineColor{.r = 0.4f, .g = 1.0f, .b = 0.5f, .a = 1.0f};

  double separatorThicknessPixels = 1.0;
  double hitLineHeight = 0.035;

  bool includeSeparators = true;
  bool includeHitLine = true;
};

[[nodiscard]] KeyboardRenderStyle keyboardRenderStyleFromSettings(const KeyboardSettings& settings);

class KeyboardRenderAdapter
{
public:
  [[nodiscard]] static std::vector<RenderCommand> buildCommands(
    const KeyboardLayoutResult& layout, const KeyboardRenderStyle& style = {});
};
