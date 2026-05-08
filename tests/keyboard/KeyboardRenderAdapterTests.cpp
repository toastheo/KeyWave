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

  REQUIRE(commands.size() == 8);
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
  checkColor(rectAt(commands, 5).color, style.blackKeyColor);
  checkColor(rectAt(commands, 6).color, style.blackKeyColor);
  checkColor(rectAt(commands, 7).color, style.hitLineColor);
  CHECK(rectAt(commands, 7).rect.x == Catch::Approx(0.0));
  CHECK(rectAt(commands, 7).rect.y == Catch::Approx(0.0));
  CHECK(rectAt(commands, 7).rect.width == Catch::Approx(layout.width));
  CHECK(rectAt(commands, 7).rect.height == Catch::Approx(style.hitLineHeight));
}

} // namespace
