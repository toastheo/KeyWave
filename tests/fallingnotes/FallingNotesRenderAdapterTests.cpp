#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <variant>
#include <vector>

#include "fallingnotes/FallingNotesRenderAdapter.hpp"
#include "render/RenderCommand.hpp"

namespace {

FallingNoteLayout makeNoteLayout(const double x,
                                 const double y,
                                 const double width,
                                 const double height,
                                 const bool clippedBottom = false,
                                 const bool clippedTop = false)
{
  return FallingNoteLayout{
    .x = x,
    .y = y,
    .width = width,
    .height = height,
    .clippedBottom = clippedBottom,
    .clippedTop = clippedTop,
  };
}

const DrawRectCommand& rectAt(const std::vector<RenderCommand>& commands, const std::size_t index)
{
  REQUIRE(index < commands.size());
  REQUIRE(std::holds_alternative<DrawRectCommand>(commands[index]));
  return std::get<DrawRectCommand>(commands[index]);
}

const DrawLineCommand& lineAt(const std::vector<RenderCommand>& commands, const std::size_t index)
{
  REQUIRE(index < commands.size());
  REQUIRE(std::holds_alternative<DrawLineCommand>(commands[index]));
  return std::get<DrawLineCommand>(commands[index]);
}

void checkColor(const Color& actual, const Color& expected)
{
  CHECK(actual.r == Catch::Approx(expected.r));
  CHECK(actual.g == Catch::Approx(expected.g));
  CHECK(actual.b == Catch::Approx(expected.b));
  CHECK(actual.a == Catch::Approx(expected.a));
}

TEST_CASE("FallingNotesRenderAdapter converts falling note layouts into draw rect commands",
          "[fallingnotes][render]")
{
  const FallingNotesLayoutResult layout{
    .notes =
      {
        makeNoteLayout(0.0, 2.0, 0.9, 1.0),
        makeNoteLayout(1.0, -0.5, 0.9, 1.0),
        makeNoteLayout(2.0, 9.5, 0.9, 0.5, false, true),
      },
    .contentWidth = 3.0,
    .contentHeight = 10.0,
  };
  constexpr FallingNotesRenderStyle style{
    .noteColor = Color{.r = 0.1f, .g = 0.2f, .b = 0.3f, .a = 0.4f},
    .activeNoteColor = Color{.r = 0.4f, .g = 0.5f, .b = 0.6f, .a = 0.7f},
    .includeOutline = false,
  };

  const auto commands = FallingNotesRenderAdapter::buildCommands(layout, style);

  REQUIRE(commands.size() == 3);

  const auto& normal = rectAt(commands, 0);
  CHECK(normal.rect.x == Catch::Approx(0.0));
  CHECK(normal.rect.y == Catch::Approx(2.0));
  CHECK(normal.rect.width == Catch::Approx(0.9));
  CHECK(normal.rect.height == Catch::Approx(1.0));
  checkColor(normal.color, style.noteColor);

  const auto& active = rectAt(commands, 1);
  CHECK(active.rect.x == Catch::Approx(1.0));
  CHECK(active.rect.y == Catch::Approx(-0.5));
  CHECK(active.rect.width == Catch::Approx(0.9));
  CHECK(active.rect.height == Catch::Approx(1.0));
  checkColor(active.color, style.activeNoteColor);

  const auto& clipped = rectAt(commands, 2);
  CHECK(clipped.rect.x == Catch::Approx(2.0));
  CHECK(clipped.rect.y == Catch::Approx(9.5));
  CHECK(clipped.rect.width == Catch::Approx(0.9));
  CHECK(clipped.rect.height == Catch::Approx(0.5));
  checkColor(clipped.color, style.noteColor);
}

TEST_CASE("FallingNotesRenderAdapter highlights notes intersecting the keyboard hit line",
          "[fallingnotes][render]")
{
  const FallingNotesLayoutResult layout{
    .notes =
      {
        makeNoteLayout(0.0, 9.5, 0.9, 0.5, false, true),
        makeNoteLayout(1.0, 0.0, 0.9, 1.0, true, false),
      },
  };
  constexpr FallingNotesRenderStyle style{
    .noteColor = Color{.r = 0.1f, .g = 0.2f, .b = 0.3f, .a = 1.0f},
    .activeNoteColor = Color{.r = 0.4f, .g = 0.5f, .b = 0.6f, .a = 1.0f},
    .includeOutline = false,
  };

  const auto commands = FallingNotesRenderAdapter::buildCommands(layout, style);

  REQUIRE(commands.size() == 2);
  checkColor(rectAt(commands, 0).color, style.noteColor);
  checkColor(rectAt(commands, 1).color, style.activeNoteColor);
}

TEST_CASE("FallingNotesRenderAdapter adds note outline line commands when enabled",
          "[fallingnotes][render]")
{
  const FallingNotesLayoutResult layout{
    .notes =
      {
        makeNoteLayout(1.0, 2.0, 3.0, 4.0),
      },
  };
  constexpr FallingNotesRenderStyle style{
    .noteColor = Color{.r = 0.1f, .g = 0.2f, .b = 0.3f, .a = 1.0f},
    .activeNoteColor = Color{.r = 0.4f, .g = 0.5f, .b = 0.6f, .a = 1.0f},
    .outlineColor = Color{.r = 0.9f, .g = 0.8f, .b = 0.7f, .a = 1.0f},
    .outlineThicknessPixels = 2.0,
    .includeOutline = true,
  };

  const auto commands = FallingNotesRenderAdapter::buildCommands(layout, style);

  REQUIRE(commands.size() == 5);
  checkColor(rectAt(commands, 0).color, style.noteColor);

  const auto& left = lineAt(commands, 1);
  CHECK(left.from.x == Catch::Approx(1.0));
  CHECK(left.from.y == Catch::Approx(2.0));
  CHECK(left.to.x == Catch::Approx(1.0));
  CHECK(left.to.y == Catch::Approx(6.0));
  CHECK(left.thickness == Catch::Approx(2.0));
  CHECK(left.cap == LineCap::Square);
  checkColor(left.color, style.outlineColor);

  const auto& right = lineAt(commands, 2);
  CHECK(right.from.x == Catch::Approx(4.0));
  CHECK(right.to.x == Catch::Approx(4.0));

  const auto& bottom = lineAt(commands, 3);
  CHECK(bottom.from.y == Catch::Approx(2.0));
  CHECK(bottom.to.y == Catch::Approx(2.0));

  const auto& top = lineAt(commands, 4);
  CHECK(top.from.y == Catch::Approx(6.0));
  CHECK(top.to.y == Catch::Approx(6.0));
}

TEST_CASE("FallingNotesRenderAdapter skips note outlines when disabled or non-positive",
          "[fallingnotes][render]")
{
  const FallingNotesLayoutResult layout{
    .notes =
      {
        makeNoteLayout(1.0, 2.0, 3.0, 4.0),
      },
  };

  FallingNotesRenderStyle style;
  style.includeOutline = false;
  style.outlineThicknessPixels = 2.0;
  CHECK(FallingNotesRenderAdapter::buildCommands(layout, style).size() == 1);

  style.includeOutline = true;
  style.outlineThicknessPixels = 0.0;
  CHECK(FallingNotesRenderAdapter::buildCommands(layout, style).size() == 1);
}

TEST_CASE("FallingNotesRenderAdapter skips invalid note rectangles", "[fallingnotes][render]")
{
  const FallingNotesLayoutResult layout{
    .notes =
      {
        makeNoteLayout(0.0, 1.0, 0.25, 0.5),
        makeNoteLayout(0.25, 2.0, 0.0, 0.5),
        makeNoteLayout(0.5, 3.0, 0.25, -0.5),
      },
  };

  FallingNotesRenderStyle style;
  style.includeOutline = false;

  const auto commands = FallingNotesRenderAdapter::buildCommands(layout, style);

  REQUIRE(commands.size() == 1);
  const auto& [rect, color] = rectAt(commands, 0);
  CHECK(rect.x == Catch::Approx(0.0));
  CHECK(rect.y == Catch::Approx(1.0));
  CHECK(rect.width == Catch::Approx(0.25));
  CHECK(rect.height == Catch::Approx(0.5));
}

TEST_CASE("FallingNotesRenderAdapter handles empty layouts gracefully", "[fallingnotes][render]")
{
  CHECK(FallingNotesRenderAdapter{}.buildCommands(FallingNotesLayoutResult{}).empty());
}

} // namespace
