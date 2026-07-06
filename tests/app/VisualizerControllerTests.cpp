#include <array>
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <string>
#include <variant>
#include <vector>

#include "app/VisualizerController.hpp"
#include "audio/PianoSynth.hpp"
#include "input/Key.hpp"
#include "midi/MidiTimeline.hpp"
#include "render/RenderCommand.hpp"

namespace {

class RecordingPianoSynth final : public PianoSynth
{
public:
  void noteOn(const PianoNote note) override
  {
    commands.push_back("on:" + std::to_string(note.pitch) + ":" + std::to_string(note.velocity));
  }

  void noteOff(const int pitch) override
  {
    commands.push_back("off:" + std::to_string(pitch));
  }

  void allNotesOff() override
  {
    commands.emplace_back("all-off");
  }

  std::vector<std::string> commands;
};

std::vector<DrawStyledRectCommand> styledRectsForScene(const RenderScene& scene)
{
  std::vector<DrawStyledRectCommand> rects;
  for (const auto& command : scene.commands) {
    if (std::holds_alternative<DrawStyledRectCommand>(command)) {
      rects.push_back(std::get<DrawStyledRectCommand>(command));
    }
  }
  return rects;
}

TEST_CASE("VisualizerController routes keyboard input around UI capture", "[app][visualizer]")
{
  VisualizerController controller;

  controller.playbackTransport().play();

  constexpr auto capturedKeys = std::array{Key::Escape, Key::Space};
  controller.handleInput(capturedKeys, true);

  CHECK_FALSE(controller.visualizationSettingsPanelVisible());
  CHECK(controller.playbackTransport().state() == PlaybackState::Playing);

  constexpr auto applicationKeys = std::array{Key::Space};
  controller.handleInput(applicationKeys, false);

  CHECK(controller.playbackTransport().state() == PlaybackState::Paused);
}

TEST_CASE("VisualizerController updates playback and builds the current scene", "[app][visualizer]")
{
  MidiTimeline timeline;
  timeline.addTempoEvent(0.0, 120.0);
  timeline.addTempoEvent(2.0, 60.0);
  timeline.addNote(Note{.pitch = 60, .velocity = 90, .startSeconds = 2.0, .durationSeconds = 1.0});

  VisualizerController controller;
  controller.setTimeline(std::move(timeline));
  controller.playbackTransport().play();

  CHECK(controller.durationSeconds() == Catch::Approx(3.0));
  CHECK(controller.sourceBpmAtPlaybackPosition() == Catch::Approx(120.0));

  controller.update(1.5);
  CHECK(controller.sourceBpmAtPlaybackPosition() == Catch::Approx(120.0));

  const auto scene = controller.buildScene();
  const auto rects = styledRectsForScene(scene);

  REQUIRE_FALSE(rects.empty());
  CHECK(rects.front().rect.y == Catch::Approx(0.5));

  controller.update(0.5);
  CHECK(controller.sourceBpmAtPlaybackPosition() == Catch::Approx(60.0));
}

TEST_CASE("VisualizerController builds an empty scene without a timeline", "[app][visualizer]")
{
  VisualizerController const controller;

  CHECK(controller.durationSeconds() == Catch::Approx(0.0));
  CHECK(controller.buildScene().empty());
}

TEST_CASE("VisualizerController replaces the timeline and starts playback from the beginning",
          "[app][visualizer]")
{
  MidiTimeline original;
  original.addNote(Note{.pitch = 60, .velocity = 90, .startSeconds = 0.0, .durationSeconds = 8.0});

  MidiTimeline replacement;
  replacement.addNote(
    Note{.pitch = 72, .velocity = 90, .startSeconds = 0.0, .durationSeconds = 2.0});

  VisualizerController controller;
  controller.setTimeline(std::move(original));
  controller.playbackTransport().play();
  controller.update(3.0);

  controller.replaceTimelineAndPlayFromStart(std::move(replacement));

  CHECK(controller.durationSeconds() == Catch::Approx(2.0));
  CHECK(controller.playbackTransport().currentTimeSeconds() == Catch::Approx(0.0));
  CHECK(controller.playbackTransport().state() == PlaybackState::Playing);

  controller.update(3.0);
  CHECK(controller.playbackTransport().currentTimeSeconds() == Catch::Approx(0.0));

  controller.update(0.25);
  CHECK(controller.playbackTransport().currentTimeSeconds() == Catch::Approx(0.25));
}

TEST_CASE("VisualizerController schedules audio while loaded timeline is playing",
          "[app][visualizer][audio]")
{
  MidiTimeline timeline;
  timeline.addNote(Note{.pitch = 60, .velocity = 90, .startSeconds = 0.25, .durationSeconds = 0.5});

  RecordingPianoSynth synth;
  VisualizerController controller(synth);

  controller.replaceTimelineAndPlayFromStart(std::move(timeline));
  controller.update(0.1);
  CHECK(synth.commands.empty());

  controller.update(0.25);
  controller.update(0.5);

  REQUIRE(synth.commands.size() == 2);
  CHECK(synth.commands[0] == "on:60:90");
  CHECK(synth.commands[1] == "off:60");
}

TEST_CASE("VisualizerController clears audio for keyboard playback controls",
          "[app][visualizer][audio]")
{
  MidiTimeline timeline;
  timeline.addNote(Note{.pitch = 60, .velocity = 90, .startSeconds = 0.25, .durationSeconds = 2.0});

  RecordingPianoSynth synth;
  VisualizerController controller(synth);

  controller.replaceTimelineAndPlayFromStart(std::move(timeline));
  controller.update(0.25);
  CHECK(synth.commands.empty());

  controller.update(0.25);
  REQUIRE(synth.commands == std::vector<std::string>{"on:60:90"});

  constexpr auto pauseKeys = std::array{Key::Space};
  controller.handleInput(pauseKeys, false);
  CHECK(controller.playbackTransport().state() == PlaybackState::Paused);
  CHECK(synth.commands.back() == "all-off");

  constexpr auto restartKeys = std::array{Key::R};
  controller.handleInput(restartKeys, false);
  CHECK(controller.playbackTransport().state() == PlaybackState::Playing);
  CHECK(controller.playbackTransport().currentTimeSeconds() == Catch::Approx(0.0));
  CHECK(synth.commands.back() == "all-off");
}

} // namespace
