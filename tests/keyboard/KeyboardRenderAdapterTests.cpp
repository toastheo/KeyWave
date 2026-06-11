#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <variant>

#include "keyboard/KeyboardLayout.hpp"
#include "keyboard/KeyboardRenderAdapter.hpp"

namespace {

const DrawRectCommand& rectAt(const std::vector<RenderCommand>& commands, const std::size_t index)
{
  REQUIRE(index < commands.size());
  REQUIRE(std::holds_alternative<DrawRectCommand>(commands[index]));
  return std::get<DrawRectCommand>(commands[index]);
}

const DrawTriangleCommand& triangleAt(const std::vector<RenderCommand>& commands,
                                      const std::size_t index)
{
  REQUIRE(index < commands.size());
  REQUIRE(std::holds_alternative<DrawTriangleCommand>(commands[index]));
  return std::get<DrawTriangleCommand>(commands[index]);
}

void checkColor(const Color& actual, const Color& expected)
{
  CHECK(actual.r == Catch::Approx(expected.r));
  CHECK(actual.g == Catch::Approx(expected.g));
  CHECK(actual.b == Catch::Approx(expected.b));
  CHECK(actual.a == Catch::Approx(expected.a));
}

TEST_CASE("KeyboardRenderAdapter emits white keys, separators, black keys, then hit line",
          "[keyboard][render]")
{
  const KeyboardGeometry geometry(KeyboardLayoutConfig{
    .pitchRange = PitchRange{.minPitch = 60, .maxPitch = 64},
  });
  const auto layout = KeyboardLayout::build(geometry);
  constexpr KeyboardRenderStyle style{
    .whiteKeyColor = Color{.r = 0.9f, .g = 0.8f, .b = 0.7f, .a = 1.0f},
    .blackKeyColor = Color{.r = 0.1f, .g = 0.1f, .b = 0.1f, .a = 1.0f},
    .whiteKeySeparatorColor = Color{.r = 0.2f, .g = 0.2f, .b = 0.2f, .a = 1.0f},
    .hitLineColor = Color{.r = 0.3f, .g = 0.4f, .b = 0.5f, .a = 1.0f},
    .separatorThicknessPixels = 1.0,
    .hitLineHeight = 0.04,
  };

  const auto commands = KeyboardRenderAdapter::buildCommands(layout, style);

  REQUIRE(commands.size() == 14);
  checkColor(rectAt(commands, 0).color, style.whiteKeyColor);
  checkColor(rectAt(commands, 1).color, style.whiteKeyColor);
  checkColor(rectAt(commands, 2).color, style.whiteKeyColor);
  REQUIRE(std::holds_alternative<DrawLineCommand>(commands[3]));
  REQUIRE(std::holds_alternative<DrawLineCommand>(commands[4]));
  const auto& firstSeparator = std::get<DrawLineCommand>(commands[3]);
  const auto& secondSeparator = std::get<DrawLineCommand>(commands[4]);
  checkColor(firstSeparator.color, style.whiteKeySeparatorColor);
  checkColor(secondSeparator.color, style.whiteKeySeparatorColor);
  CHECK(firstSeparator.from.x == Catch::Approx(1.0));
  CHECK(firstSeparator.to.x == Catch::Approx(1.0));
  CHECK(firstSeparator.from.y == Catch::Approx(-layout.height));
  CHECK(firstSeparator.to.y == Catch::Approx(0.0));
  CHECK(firstSeparator.thickness == Catch::Approx(style.separatorThicknessPixels));
  REQUIRE(std::holds_alternative<DrawTriangleCommand>(commands[5]));
  REQUIRE(std::holds_alternative<DrawTriangleCommand>(commands[6]));
  REQUIRE(std::holds_alternative<DrawTriangleCommand>(commands[7]));
  REQUIRE(std::holds_alternative<DrawTriangleCommand>(commands[8]));
  REQUIRE(std::holds_alternative<DrawTriangleCommand>(commands[9]));
  REQUIRE(std::holds_alternative<DrawTriangleCommand>(commands[10]));
  checkColor(rectAt(commands, 11).color, style.blackKeyColor);
  checkColor(rectAt(commands, 12).color, style.blackKeyColor);
  checkColor(rectAt(commands, 13).color, style.hitLineColor);
  CHECK(rectAt(commands, 13).rect.x == Catch::Approx(0.0));
  CHECK(rectAt(commands, 13).rect.y == Catch::Approx(0.0));
  CHECK(rectAt(commands, 13).rect.width == Catch::Approx(layout.width));
  CHECK(rectAt(commands, 13).rect.height == Catch::Approx(style.hitLineHeight));
}

TEST_CASE("KeyboardRenderAdapter emits aligned black bottom cut masks for white keys",
          "[keyboard][render]")
{
  KeyboardLayoutResult const layout{
    .whiteKeys =
      {
        PianoKeyLayout{
          .pitch = 60,
          .kind = PianoKeyKind::White,
          .rect = Rect{.x = 2.0, .y = -2.5, .width = 1.0, .height = 2.5},
          .active = false,
          .velocity = 0,
        },
      },
    .blackKeys = {},
    .pitchRange = PitchRange{.minPitch = 60, .maxPitch = 60},
    .width = 1.0,
    .height = 2.5,
  };
  constexpr KeyboardRenderStyle style{
    .includeSeparators = false,
    .includeHitLine = false,
  };

  const auto commands = KeyboardRenderAdapter::buildCommands(layout, style);

  REQUIRE(commands.size() == 3);
  REQUIRE(std::holds_alternative<DrawRectCommand>(commands[0]));
  const auto& leftCut = triangleAt(commands, 1);
  const auto& rightCut = triangleAt(commands, 2);

  checkColor(leftCut.color, Color{.r = 0.0f, .g = 0.0f, .b = 0.0f, .a = 1.0f});
  checkColor(rightCut.color, Color{.r = 0.0f, .g = 0.0f, .b = 0.0f, .a = 1.0f});

  CHECK(leftCut.a.x == Catch::Approx(2.0));
  CHECK(leftCut.a.y == Catch::Approx(-2.5));
  CHECK(leftCut.b.x == Catch::Approx(2.0));
  CHECK(leftCut.b.y == Catch::Approx(-2.45));
  CHECK(leftCut.c.x == Catch::Approx(2.12));
  CHECK(leftCut.c.y == Catch::Approx(-2.5));

  CHECK(rightCut.a.x == Catch::Approx(3.0));
  CHECK(rightCut.a.y == Catch::Approx(-2.5));
  CHECK(rightCut.b.x == Catch::Approx(3.0));
  CHECK(rightCut.b.y == Catch::Approx(-2.45));
  CHECK(rightCut.c.x == Catch::Approx(2.88));
  CHECK(rightCut.c.y == Catch::Approx(-2.5));
}

TEST_CASE("KeyboardRenderAdapter skips bottom cut masks for invalid white key rectangles",
          "[keyboard][render]")
{
  KeyboardLayoutResult const layout{
    .whiteKeys =
      {
        PianoKeyLayout{
          .pitch = 60,
          .kind = PianoKeyKind::White,
          .rect = Rect{.x = 2.0, .y = -2.5, .width = 0.0, .height = 2.5},
          .active = false,
          .velocity = 0,
        },
      },
    .blackKeys = {},
    .pitchRange = PitchRange{.minPitch = 60, .maxPitch = 60},
    .width = 1.0,
    .height = 2.5,
  };
  constexpr KeyboardRenderStyle style{
    .includeSeparators = false,
    .includeHitLine = false,
  };

  const auto commands = KeyboardRenderAdapter::buildCommands(layout, style);

  CHECK(commands.empty());
}

TEST_CASE("KeyboardRenderAdapter uses active colors for active keys", "[keyboard][render]")
{
  KeyboardLayoutResult const layout{
    .whiteKeys =
      {
        PianoKeyLayout{
          .pitch = 60,
          .kind = PianoKeyKind::White,
          .rect = Rect{.x = 0.0, .y = -2.5, .width = 1.0, .height = 2.5},
          .active = true,
          .velocity = 90,
        },
      },
    .blackKeys =
      {
        PianoKeyLayout{
          .pitch = 61,
          .kind = PianoKeyKind::Black,
          .rect = Rect{.x = 0.7, .y = -1.55, .width = 0.6, .height = 1.55},
          .active = true,
          .velocity = 80,
        },
      },
    .pitchRange = PitchRange{.minPitch = 60, .maxPitch = 61},
    .width = 1.0,
    .height = 2.5,
  };
  constexpr KeyboardRenderStyle style{
    .whiteKeyColor = Color{.r = 0.9f, .g = 0.8f, .b = 0.7f, .a = 1.0f},
    .blackKeyColor = Color{.r = 0.1f, .g = 0.1f, .b = 0.1f, .a = 1.0f},
    .activeWhiteKeyColor = Color{.r = 0.2f, .g = 0.9f, .b = 1.0f, .a = 1.0f},
    .activeBlackKeyColor = Color{.r = 0.0f, .g = 0.5f, .b = 1.0f, .a = 1.0f},
    .includeSeparators = false,
    .includeHitLine = false,
  };

  const auto commands = KeyboardRenderAdapter::buildCommands(layout, style);

  REQUIRE(commands.size() == 4);
  checkColor(rectAt(commands, 0).color, style.activeWhiteKeyColor);
  REQUIRE(std::holds_alternative<DrawTriangleCommand>(commands[1]));
  REQUIRE(std::holds_alternative<DrawTriangleCommand>(commands[2]));
  checkColor(rectAt(commands, 3).color, style.activeBlackKeyColor);
}

} // namespace
