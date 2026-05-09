#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <limits>
#include <variant>

#include "fallingnotes/FallingNotesSceneBuilder.hpp"
#include "midi/MidiTimeline.hpp"
#include "render/RenderCommand.hpp"

namespace {

std::vector<DrawRectCommand> rectsForScene(const RenderScene& scene)
{
  std::vector<DrawRectCommand> rects;
  for (const auto& command : scene.commands) {
    if (std::holds_alternative<DrawRectCommand>(command)) {
      rects.push_back(std::get<DrawRectCommand>(command));
    }
  }
  return rects;
}

TEST_CASE("FallingNotesSceneBuilder rebuilds note positions for the current playback time",
          "[fallingnotes][scene]")
{
  MidiTimeline timeline;
  timeline.addNote(Note{.pitch = 60, .velocity = 90, .startSeconds = 2.0, .durationSeconds = 1.0});

  const auto firstScene = FallingNotesSceneBuilder::build(timeline, 0.0);
  const auto laterScene = FallingNotesSceneBuilder::build(timeline, 1.5);

  CHECK(firstScene.view.visibleWorldRect.x == Catch::Approx(0.0));
  CHECK(firstScene.view.visibleWorldRect.y == Catch::Approx(-2.5));
  CHECK(firstScene.view.visibleWorldRect.width == Catch::Approx(52.0));
  CHECK(firstScene.view.visibleWorldRect.height == Catch::Approx(12.5));

  const auto firstRects = rectsForScene(firstScene);
  const auto laterRects = rectsForScene(laterScene);
  REQUIRE_FALSE(firstRects.empty());
  REQUIRE_FALSE(laterRects.empty());
  CHECK(firstRects.front().rect.y == Catch::Approx(2.0));
  CHECK(laterRects.front().rect.y == Catch::Approx(0.5));
  CHECK(firstRects.back().rect.y == Catch::Approx(0.0));
  CHECK(firstRects.back().rect.width == Catch::Approx(52.0));
}

TEST_CASE("FallingNotesSceneBuilder returns a default scene when layout input is invalid",
          "[fallingnotes][scene]")
{
  MidiTimeline timeline;
  timeline.addNote(Note{.pitch = 60, .velocity = 90, .startSeconds = 2.0, .durationSeconds = 1.0});

  const auto scene =
    FallingNotesSceneBuilder::build(timeline, std::numeric_limits<double>::quiet_NaN());

  CHECK_FALSE(scene.commands.empty());
  CHECK(scene.view.visibleWorldRect.width == Catch::Approx(kDefaultVisibleWorldRect.width));
  CHECK(scene.view.visibleWorldRect.height == Catch::Approx(kDefaultVisibleWorldRect.height));
}

} // namespace
