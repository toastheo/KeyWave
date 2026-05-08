#pragma once

#include "midi/MidiTimeline.hpp"
#include "render/RenderScene.hpp"

class FallingNotesSceneBuilder
{
public:
  [[nodiscard]] static RenderScene build(const MidiTimeline& timeline, double currentTimeSeconds);
};
