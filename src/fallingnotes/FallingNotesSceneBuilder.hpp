#pragma once

#include "diagnostics/Diagnostics.hpp"
#include "fallingnotes/FallingNotesRenderAdapter.hpp"
#include "keyboard/KeyboardRenderAdapter.hpp"
#include "keyboard/KeyboardTypes.hpp"
#include "midi/MidiTimeline.hpp"
#include "render/RenderScene.hpp"

struct FallingNotesSceneConfig
{
  PitchRange pitchRange{.minPitch = 21, .maxPitch = 108};
  double lookAheadSeconds = 10.0;
  double visiblePastSeconds = 0.0;
  double displayHeight = 10.0;
  KeyboardLayoutConfig keyboardLayout;
  FallingNotesRenderStyle fallingNotesStyle;
  KeyboardRenderStyle keyboardStyle;
};

class FallingNotesSceneBuilder
{
public:
  [[nodiscard]] static RenderScene build(const MidiTimeline& timeline,
                                         double currentTimeSeconds,
                                         const FallingNotesSceneConfig& config = {},
                                         DiagnosticSink& diagnostics = nullDiagnosticSink());
};
