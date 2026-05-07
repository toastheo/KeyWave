#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <variant>
#include <vector>

#include "pianoroll/PianoRollRenderAdapter.hpp"
#include "render/RenderCommand.hpp"

namespace {

PianoRollNoteLayout makeNoteLayout(const double x,
                                   const double y,
                                   const double width,
                                   const double height,
                                   const bool clippedLeft = false,
                                   const bool clippedRight = false)
{
  return PianoRollNoteLayout{
    .x = x,
    .y = y,
    .width = width,
    .height = height,
    .clippedLeft = clippedLeft,
    .clippedRight = clippedRight,
  };
}

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

TEST_CASE("PianoRollRenderAdapter converts note layouts into draw rect commands",
          "[pianoroll][render]")
{
  const PianoRollLayoutResult layout{
    .notes =
      {
        makeNoteLayout(0.1, 2.0, 0.25, 0.75),
        makeNoteLayout(0.5, 4.0, 0.125, 0.5, true, false),
        makeNoteLayout(0.8, 7.0, 0.1, 0.4, false, true),
      },
    .contentWidth = 1.0,
    .contentHeight = 88.0,
  };
  constexpr PianoRollRenderStyle style{
    .noteColor = Color{.r = 0.1f, .g = 0.2f, .b = 0.3f, .a = 0.4f},
    .clippedNoteColor = Color{.r = 0.9f, .g = 0.8f, .b = 0.7f, .a = 0.6f},
  };

  const auto commands = PianoRollRenderAdapter::buildCommands(layout, style);

  REQUIRE(commands.size() == 3);

  const auto& [rect, color] = rectAt(commands, 0);
  CHECK(rect.x == Catch::Approx(0.1));
  CHECK(rect.y == Catch::Approx(2.0));
  CHECK(rect.width == Catch::Approx(0.25));
  CHECK(rect.height == Catch::Approx(0.75));
  checkColor(color, style.noteColor);

  const auto& second = rectAt(commands, 1);
  CHECK(second.rect.x == Catch::Approx(0.5));
  CHECK(second.rect.y == Catch::Approx(4.0));
  CHECK(second.rect.width == Catch::Approx(0.125));
  CHECK(second.rect.height == Catch::Approx(0.5));
  checkColor(second.color, style.clippedNoteColor);

  const auto& third = rectAt(commands, 2);
  CHECK(third.rect.x == Catch::Approx(0.8));
  CHECK(third.rect.y == Catch::Approx(7.0));
  CHECK(third.rect.width == Catch::Approx(0.1));
  CHECK(third.rect.height == Catch::Approx(0.4));
  checkColor(third.color, style.clippedNoteColor);
}

TEST_CASE("PianoRollRenderAdapter can prepend a background command", "[pianoroll][render]")
{
  const PianoRollLayoutResult layout{
    .notes = {makeNoteLayout(0.25, 12.0, 0.5, 0.75)},
    .contentWidth = 2.0,
    .contentHeight = 24.0,
  };
  constexpr PianoRollRenderStyle style{
    .backgroundColor = Color{.r = 0.05f, .g = 0.06f, .b = 0.07f, .a = 1.0f},
    .includeBackground = true,
  };

  const auto commands = PianoRollRenderAdapter::buildCommands(layout, style);

  REQUIRE(commands.size() == 2);

  const auto& [rect, color] = rectAt(commands, 0);
  CHECK(rect.x == Catch::Approx(0.0));
  CHECK(rect.y == Catch::Approx(0.0));
  CHECK(rect.width == Catch::Approx(2.0));
  CHECK(rect.height == Catch::Approx(24.0));
  checkColor(color, style.backgroundColor);

  const auto& note = rectAt(commands, 1);
  CHECK(note.rect.x == Catch::Approx(0.25));
  CHECK(note.rect.y == Catch::Approx(12.0));
}

TEST_CASE("PianoRollRenderAdapter skips invalid note rectangles", "[pianoroll][render]")
{
  const PianoRollLayoutResult layout{
    .notes =
      {
        makeNoteLayout(0.0, 1.0, 0.25, 0.5),
        makeNoteLayout(0.25, 2.0, 0.0, 0.5),
        makeNoteLayout(0.5, 3.0, 0.25, -0.5),
      },
  };

  const auto commands = PianoRollRenderAdapter::buildCommands(layout);

  REQUIRE(commands.size() == 1);
  const auto& [rect, color] = rectAt(commands, 0);
  CHECK(rect.x == Catch::Approx(0.0));
  CHECK(rect.y == Catch::Approx(1.0));
  CHECK(rect.width == Catch::Approx(0.25));
  CHECK(rect.height == Catch::Approx(0.5));
}

TEST_CASE("PianoRollRenderAdapter handles empty layouts gracefully", "[pianoroll][render]")
{
  constexpr PianoRollRenderAdapter adapter;

  CHECK(adapter.buildCommands(PianoRollLayoutResult{}).empty());
  CHECK(adapter
          .buildCommands(PianoRollLayoutResult{},
                         PianoRollRenderStyle{
                           .includeBackground = true,
                         })
          .empty());
}

} // namespace
