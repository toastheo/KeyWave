#pragma once

#include "app/AppSettings.hpp"
#include "fallingnotes/FallingNotesSceneBuilder.hpp"
#include "keyboard/KeyboardTypes.hpp"

[[nodiscard]] KeyboardLayoutConfig keyboardLayoutConfigFromSettings(
  const KeyboardSettings& settings, const PitchRange& pitchRange);
[[nodiscard]] KeyboardRenderStyle keyboardRenderStyleFromSettings(const KeyboardSettings& settings);
[[nodiscard]] FallingNotesRenderStyle fallingNotesRenderStyleFromSettings(
  const FallingNotesSettings& settings);
[[nodiscard]] FallingNotesSceneConfig fallingNotesSceneConfigFromSettings(
  const FallingNotesSettings& fallingNotesSettings, const KeyboardSettings& keyboardSettings);
