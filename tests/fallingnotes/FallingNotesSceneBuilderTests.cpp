#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <limits>
#include <variant>

#include "app/AppSettings.hpp"
#include "fallingnotes/FallingNotesSceneBuilder.hpp"
#include "keyboard/KeyboardRenderAdapter.hpp"
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

bool sameColor(const Color& left, const Color& right)
{
  return left.r == Catch::Approx(right.r) && left.g == Catch::Approx(right.g) &&
         left.b == Catch::Approx(right.b) && left.a == Catch::Approx(right.a);
}

void checkColor(const Color& actual, const Color& expected)
{
  CHECK(sameColor(actual, expected));
}

bool isKeyboardRect(const DrawRectCommand& command)
{
  return command.rect.y < 0.0;
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

TEST_CASE("FallingNotesSceneBuilder highlights keyboard keys for currently active MIDI notes",
          "[fallingnotes][scene]")
{
  MidiTimeline timeline;
  timeline.addNote(Note{.pitch = 60, .velocity = 90, .startSeconds = 1.0, .durationSeconds = 2.0});
  timeline.addNote(Note{.pitch = 61, .velocity = 90, .startSeconds = 1.5, .durationSeconds = 2.0});
  timeline.addNote(Note{.pitch = 64, .velocity = 90, .startSeconds = 4.0, .durationSeconds = 1.0});

  const auto scene = FallingNotesSceneBuilder::build(timeline, 2.0);
  const auto rects = rectsForScene(scene);

  int activeWhiteKeyCount = 0;
  int activeBlackKeyCount = 0;
  for (const auto& rect : rects) {
    constexpr KeyboardRenderStyle keyboardStyle;

    if (isKeyboardRect(rect) && sameColor(rect.color, keyboardStyle.activeWhiteKeyColor)) {
      ++activeWhiteKeyCount;
    }
    if (isKeyboardRect(rect) && sameColor(rect.color, keyboardStyle.activeBlackKeyColor)) {
      ++activeBlackKeyCount;
    }
  }

  CHECK(activeWhiteKeyCount == 1);
  CHECK(activeBlackKeyCount == 1);
}

TEST_CASE("FallingNotesSceneBuilder uses falling note and keyboard settings",
          "[fallingnotes][scene]")
{
  MidiTimeline timeline;
  timeline.addNote(Note{.pitch = 60, .velocity = 90, .startSeconds = 9.5, .durationSeconds = 1.0});
  timeline.addNote(Note{.pitch = 72, .velocity = 90, .startSeconds = 10.0, .durationSeconds = 1.0});

  constexpr FallingNotesSettings fallingNotesSettings{
    .pitchRange = PitchRange{.minPitch = 60, .maxPitch = 64},
    .lookAheadSeconds = 4.0,
    .visiblePastSeconds = 1.0,
    .noteColor = Color{.r = 0.7f, .g = 0.2f, .b = 0.1f, .a = 1.0f},
    .activeNoteColor = Color{.r = 0.1f, .g = 0.9f, .b = 0.2f, .a = 1.0f},
  };
  constexpr KeyboardSettings keyboardSettings{
    .whiteKeyWidth = 2.0,
    .whiteKeyHeight = 1.25,
    .blackKeyWidth = 1.0,
    .blackKeyHeight = 0.75,
    .whiteKeyGap = 0.2,
    .whiteKeyColor = Color{.r = 0.9f, .g = 0.8f, .b = 0.7f, .a = 1.0f},
    .blackKeyColor = Color{.r = 0.1f, .g = 0.1f, .b = 0.1f, .a = 1.0f},
    .activeWhiteKeyColor = Color{.r = 0.2f, .g = 0.8f, .b = 0.9f, .a = 1.0f},
    .activeBlackKeyColor = Color{.r = 0.2f, .g = 0.4f, .b = 0.9f, .a = 1.0f},
    .whiteKeySeparatorColor = Color{.r = 0.25f, .g = 0.25f, .b = 0.27f, .a = 1.0f},
    .hitLineColor = Color{.r = 0.6f, .g = 0.6f, .b = 0.9f, .a = 1.0f},
    .separatorWidth = 2.0,
    .hitLineHeight = 0.05,
  };

  const auto scene =
    FallingNotesSceneBuilder::build(timeline, 10.0, fallingNotesSettings, keyboardSettings);
  const auto rects = rectsForScene(scene);

  CHECK(scene.view.visibleWorldRect.y == Catch::Approx(-1.25));
  CHECK(scene.view.visibleWorldRect.width == Catch::Approx(6.0));
  CHECK(scene.view.visibleWorldRect.height == Catch::Approx(11.25));

  REQUIRE_FALSE(rects.empty());
  checkColor(rects.front().color, fallingNotesSettings.activeNoteColor);
  CHECK(rects.front().rect.x == Catch::Approx(0.1));
  CHECK(rects.front().rect.width == Catch::Approx(1.8));
  CHECK(rects.back().rect.height == Catch::Approx(keyboardSettings.hitLineHeight));
  checkColor(rects.back().color, keyboardSettings.hitLineColor);
}

TEST_CASE("FallingNotesSceneBuilder keeps keyboard view size independent from lookahead",
          "[fallingnotes][scene]")
{
  MidiTimeline timeline;
  timeline.addNote(Note{.pitch = 60, .velocity = 90, .startSeconds = 18.0, .durationSeconds = 1.0});

  FallingNotesSettings shortLookaheadSettings;
  shortLookaheadSettings.lookAheadSeconds = 4.0;

  FallingNotesSettings longLookaheadSettings;
  longLookaheadSettings.lookAheadSeconds = 20.0;

  constexpr KeyboardSettings keyboardSettings;

  const auto shortScene =
    FallingNotesSceneBuilder::build(timeline, 0.0, shortLookaheadSettings, keyboardSettings);
  const auto longScene =
    FallingNotesSceneBuilder::build(timeline, 0.0, longLookaheadSettings, keyboardSettings);

  CHECK(shortScene.view.visibleWorldRect.y == Catch::Approx(longScene.view.visibleWorldRect.y));
  CHECK(shortScene.view.visibleWorldRect.height ==
        Catch::Approx(longScene.view.visibleWorldRect.height));

  const auto longRects = rectsForScene(longScene);
  REQUIRE_FALSE(longRects.empty());
  CHECK(longRects.front().rect.y >= 0.0);
  CHECK(longRects.front().rect.y + longRects.front().rect.height <=
        longScene.view.visibleWorldRect.y + longScene.view.visibleWorldRect.height);
}

} // namespace
