#pragma once

#include <string>

#include "midi/MidiTypes.hpp"
#include "render/RenderTypes.hpp"

struct WindowSettings
{
  std::string title = "KeyWave";
  int width = 1280;
  int height = 720;
};

struct RendererSettings
{
  Color clearColor{.r = 0.025f, .g = 0.03f, .b = 0.04f, .a = 1.0f};
};

struct PlaybackControlSettings
{
  double seekStepSeconds = 5.0;
  double minPlaybackRate = 0.25;
  double maxPlaybackRate = 4.0;
  double playbackRateStep = 0.25;
};

struct FallingNotesSettings
{
  PitchRange pitchRange{.minPitch = 21, .maxPitch = 108};
  double lookAheadSeconds = 10.0;
  double visiblePastSeconds = 0.0;

  Color noteColor{.r = 0.2f, .g = 0.7f, .b = 1.0f, .a = 1.0f};
  Color activeNoteColor{.r = 0.4f, .g = 1.0f, .b = 0.5f, .a = 1.0f};
  Color clippedNoteColor{.r = 1.0f, .g = 0.6f, .b = 0.2f, .a = 1.0f};
};

struct KeyboardSettings
{
  double whiteKeyWidth = 1.0;
  double whiteKeyHeight = 2.5;
  double blackKeyWidth = 0.6;
  double blackKeyHeight = 1.55;
  double whiteKeyGap = 0.015;

  Color whiteKeyColor{.r = 0.92f, .g = 0.92f, .b = 0.90f, .a = 1.0f};
  Color blackKeyColor{.r = 0.04f, .g = 0.04f, .b = 0.05f, .a = 1.0f};
  Color activeWhiteKeyColor{.r = 0.4f, .g = 1.0f, .b = 0.5f, .a = 1.0f};
  Color activeBlackKeyColor{.r = 0.2f, .g = 0.68f, .b = 0.28f, .a = 1.0f};
  Color whiteKeySeparatorColor{.r = 0.25f, .g = 0.25f, .b = 0.27f, .a = 1.0f};
  Color hitLineColor{.r = 0.4f, .g = 1.0f, .b = 0.5f, .a = 1.0f};

  double separatorWidth = 1.0;
  double hitLineHeight = 0.035;

  bool includeSeparators = true;
  bool includeHitLine = true;
};

struct AppSettings
{
  WindowSettings window;
  RendererSettings renderer;
  PlaybackControlSettings playbackControls;
  FallingNotesSettings fallingNotes;
  KeyboardSettings keyboard;
};

[[nodiscard]] WindowSettings sanitizeWindowSettings(WindowSettings settings);
[[nodiscard]] PlaybackControlSettings sanitizePlaybackControlSettings(
  PlaybackControlSettings settings);
[[nodiscard]] FallingNotesSettings sanitizeFallingNotesSettings(FallingNotesSettings settings);
[[nodiscard]] KeyboardSettings sanitizeKeyboardSettings(KeyboardSettings settings);
[[nodiscard]] AppSettings sanitizeAppSettings(AppSettings settings);
