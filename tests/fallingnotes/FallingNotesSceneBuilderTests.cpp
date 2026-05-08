#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <limits>
#include <variant>

#include "fallingnotes/FallingNotesSceneBuilder.hpp"
#include "midi/MidiTimeline.hpp"
#include "render/RenderCommand.hpp"

namespace {

const DrawRectCommand& onlyRect(const RenderScene& scene)
{
  REQUIRE(scene.commands.size() == 1);
  REQUIRE(std::holds_alternative<DrawRectCommand>(scene.commands.front()));
  return std::get<DrawRectCommand>(scene.commands.front());
}

TEST_CASE("FallingNotesSceneBuilder rebuilds note positions for the current playback time",
          "[fallingnotes][scene]")
{
  MidiTimeline timeline;
  timeline.addNote(Note{.pitch = 60, .velocity = 90, .startSeconds = 2.0, .durationSeconds = 1.0});

  const auto firstScene = FallingNotesSceneBuilder::build(timeline, 0.0);
  const auto laterScene = FallingNotesSceneBuilder::build(timeline, 1.5);

  CHECK(firstScene.view.visibleWorldRect.x == Catch::Approx(0.0));
  CHECK(firstScene.view.visibleWorldRect.y == Catch::Approx(0.0));
  CHECK(firstScene.view.visibleWorldRect.width == Catch::Approx(88.0));
  CHECK(firstScene.view.visibleWorldRect.height == Catch::Approx(10.0));

  CHECK(onlyRect(firstScene).rect.y == Catch::Approx(2.0));
  CHECK(onlyRect(laterScene).rect.y == Catch::Approx(0.5));
}

TEST_CASE("FallingNotesSceneBuilder returns a default scene when layout input is invalid",
          "[fallingnotes][scene]")
{
  MidiTimeline timeline;
  timeline.addNote(Note{.pitch = 60, .velocity = 90, .startSeconds = 2.0, .durationSeconds = 1.0});

  const auto scene =
    FallingNotesSceneBuilder::build(timeline, std::numeric_limits<double>::quiet_NaN());

  CHECK(scene.commands.empty());
  CHECK(scene.view.visibleWorldRect.width == Catch::Approx(kDefaultVisibleWorldRect.width));
  CHECK(scene.view.visibleWorldRect.height == Catch::Approx(kDefaultVisibleWorldRect.height));
}

} // namespace

