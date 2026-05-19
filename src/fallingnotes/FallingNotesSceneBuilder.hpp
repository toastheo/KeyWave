#pragma once

#include "app/AppSettings.hpp"
#include "midi/MidiTimeline.hpp"
#include "render/RenderScene.hpp"

class FallingNotesSceneBuilder
{
public:
  [[nodiscard]] static RenderScene build(const MidiTimeline& timeline,
                                         double currentTimeSeconds,
                                         const FallingNotesSettings& fallingNotesSettings = {},
                                         const KeyboardSettings& keyboardSettings = {});
};
