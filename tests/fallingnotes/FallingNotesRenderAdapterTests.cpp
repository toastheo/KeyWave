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

const DrawStyledRectCommand& styledRectAt(const std::vector<RenderCommand>& commands,
                                          const std::size_t index)
{
  REQUIRE(index < commands.size());
  REQUIRE(std::holds_alternative<DrawStyledRectCommand>(commands[index]));
  return std::get<DrawStyledRectCommand>(commands[index]);
}

void checkColor(const Color& actual, const Color& expected)
{
  CHECK(actual.r == Catch::Approx(expected.r));
  CHECK(actual.g == Catch::Approx(expected.g));
  CHECK(actual.b == Catch::Approx(expected.b));
  CHECK(actual.a == Catch::Approx(expected.a));
}

void checkCornerRadii(const DrawStyledRectCommand& command, const CornerRadiiPixels& expected)
{
  CHECK(command.cornerRadiiPixels.topLeft == Catch::Approx(expected.topLeft));
  CHECK(command.cornerRadiiPixels.topRight == Catch::Approx(expected.topRight));
  CHECK(command.cornerRadiiPixels.bottomRight == Catch::Approx(expected.bottomRight));
  CHECK(command.cornerRadiiPixels.bottomLeft == Catch::Approx(expected.bottomLeft));
}

Color bottomGradientColorFor(const Color color)
{
  return Color{.r = color.r * 0.7f, .g = color.g * 0.7f, .b = color.b * 0.7f, .a = color.a};
}

TEST_CASE("FallingNotesRenderAdapter converts falling note layouts into styled rect commands",
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

  const auto& normal = styledRectAt(commands, 0);
  CHECK(normal.rect.x == Catch::Approx(0.0));
  CHECK(normal.rect.y == Catch::Approx(2.0));
  CHECK(normal.rect.width == Catch::Approx(0.9));
  CHECK(normal.rect.height == Catch::Approx(1.0));
  checkColor(normal.topColor, style.noteColor);
  checkColor(normal.bottomColor, bottomGradientColorFor(style.noteColor));
  CHECK(normal.borderThicknessPixels == Catch::Approx(0.0));

  const auto& active = styledRectAt(commands, 1);
  CHECK(active.rect.x == Catch::Approx(1.0));
  CHECK(active.rect.y == Catch::Approx(-0.5));
  CHECK(active.rect.width == Catch::Approx(0.9));
  CHECK(active.rect.height == Catch::Approx(1.0));
  checkColor(active.topColor, style.activeNoteColor);
  checkColor(active.bottomColor, bottomGradientColorFor(style.activeNoteColor));
  CHECK(active.borderThicknessPixels == Catch::Approx(0.0));

  const auto& clipped = styledRectAt(commands, 2);
  CHECK(clipped.rect.x == Catch::Approx(2.0));
  CHECK(clipped.rect.y == Catch::Approx(9.5));
  CHECK(clipped.rect.width == Catch::Approx(0.9));
  CHECK(clipped.rect.height == Catch::Approx(0.5));
  checkColor(clipped.topColor, style.noteColor);
  checkColor(clipped.bottomColor, bottomGradientColorFor(style.noteColor));
  CHECK(clipped.borderThicknessPixels == Catch::Approx(0.0));
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
  checkColor(styledRectAt(commands, 0).topColor, style.noteColor);
  checkColor(styledRectAt(commands, 1).topColor, style.activeNoteColor);
}

TEST_CASE("FallingNotesRenderAdapter adds note border styling when enabled",
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
    .cornerRadiusPixels = 7.0,
    .includeOutline = true,
  };

  const auto commands = FallingNotesRenderAdapter::buildCommands(layout, style);

  REQUIRE(commands.size() == 1);
  const auto& note = styledRectAt(commands, 0);
  checkColor(note.topColor, style.noteColor);
  checkColor(note.borderColor, style.outlineColor);
  CHECK(note.borderThicknessPixels == Catch::Approx(2.0));
  checkCornerRadii(note,
                   CornerRadiiPixels{
                     .topLeft = 7.0,
                     .topRight = 7.0,
                     .bottomRight = 7.0,
                     .bottomLeft = 7.0,
                   });
}

TEST_CASE("FallingNotesRenderAdapter keeps clipped note corners rounded",
          "[fallingnotes][render]")
{
  const FallingNotesLayoutResult layout{
    .notes =
      {
        makeNoteLayout(1.0, 2.0, 3.0, 4.0),
        makeNoteLayout(1.0, 8.0, 3.0, 2.0, false, true),
        makeNoteLayout(1.0, 0.0, 3.0, 2.0, true, false),
      },
  };
  constexpr FallingNotesRenderStyle style{
    .cornerRadiusPixels = 7.0,
  };

  const auto commands = FallingNotesRenderAdapter::buildCommands(layout, style);

  REQUIRE(commands.size() == 3);
  checkCornerRadii(styledRectAt(commands, 0),
                   CornerRadiiPixels{
                     .topLeft = 7.0,
                     .topRight = 7.0,
                     .bottomRight = 7.0,
                     .bottomLeft = 7.0,
                   });
  checkCornerRadii(styledRectAt(commands, 1),
                   CornerRadiiPixels{
                     .topLeft = 7.0,
                     .topRight = 7.0,
                     .bottomRight = 7.0,
                     .bottomLeft = 7.0,
                   });
  checkCornerRadii(styledRectAt(commands, 2),
                   CornerRadiiPixels{
                     .topLeft = 7.0,
                     .topRight = 7.0,
                     .bottomRight = 7.0,
                     .bottomLeft = 7.0,
                   });
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
  auto commands = FallingNotesRenderAdapter::buildCommands(layout, style);
  REQUIRE(commands.size() == 1);
  CHECK(styledRectAt(commands, 0).borderThicknessPixels == Catch::Approx(0.0));

  style.includeOutline = true;
  style.outlineThicknessPixels = 0.0;
  commands = FallingNotesRenderAdapter::buildCommands(layout, style);
  REQUIRE(commands.size() == 1);
  CHECK(styledRectAt(commands, 0).borderThicknessPixels == Catch::Approx(0.0));
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
  const auto& rect = styledRectAt(commands, 0).rect;
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
