#include <array>
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <sstream>
#include <variant>

#include "app/VisualizerController.hpp"
#include "input/Key.hpp"
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

TEST_CASE("VisualizerController routes keyboard input around UI capture", "[app][visualizer]")
{
  VisualizerController controller;
  std::ostringstream log;

  controller.playbackTransport().play();

  constexpr auto capturedKeys = std::array{Key::Escape, Key::Space};
  controller.handleInput(capturedKeys, true, log);

  CHECK_FALSE(controller.visualizationSettingsPanelVisible());
  CHECK(controller.playbackTransport().state() == PlaybackState::Playing);

  constexpr auto applicationKeys = std::array{Key::Space};
  controller.handleInput(applicationKeys, false, log);

  CHECK(controller.playbackTransport().state() == PlaybackState::Paused);
}

TEST_CASE("VisualizerController updates playback and builds the current scene", "[app][visualizer]")
{
  MidiTimeline timeline;
  timeline.addNote(Note{.pitch = 60, .velocity = 90, .startSeconds = 2.0, .durationSeconds = 1.0});

  VisualizerController controller;
  controller.setTimeline(std::move(timeline));
  controller.playbackTransport().play();

  CHECK(controller.durationSeconds() == Catch::Approx(3.0));

  controller.update(1.5);
  const auto scene = controller.buildScene();
  const auto rects = rectsForScene(scene);

  REQUIRE_FALSE(rects.empty());
  CHECK(rects.front().rect.y == Catch::Approx(0.5));
}

TEST_CASE("VisualizerController builds an empty scene without a timeline", "[app][visualizer]")
{
  VisualizerController const controller;

  CHECK(controller.durationSeconds() == Catch::Approx(0.0));
  CHECK(controller.buildScene().empty());
}

} // namespace
