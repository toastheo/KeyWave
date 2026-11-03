#pragma once

#include "core/CoreTypes.hpp"
#include "diagnostics/Diagnostics.hpp"
#include "fallingnotes/PianoRollSceneBuilder.hpp"
#include "midi/MidiTimeline.hpp"
#include "render/RenderScene.hpp"
#include "render/RendererView.hpp"

struct ExportComposition
{
  PianoRollSceneConfig sceneConfig;
  RendererView view;
};

struct ExportCompositionOptions
{
  // Freeze the actual live framebuffer aspect, including manual window resizing.
  // CPU-only callers without a live window use the application's default 16:9 aspect.
  FramebufferSize sourceFramebufferSize{.width = 1920, .height = 1080};
};

// Pixel styles use a 1080-pixel short-side reference. Use the actual rendering
// size here, including for previews; world geometry depends only on aspect ratio.
[[nodiscard]] ExportComposition exportComposition(const PianoRollSceneConfig& config,
                                                  FramebufferSize size,
                                                  ExportCompositionOptions options = {});

[[nodiscard]] RenderScene buildExportScene(const MidiTimeline& timeline,
                                           double sourceSeconds,
                                           const PianoRollSceneConfig& config,
                                           FramebufferSize size,
                                           ExportCompositionOptions options = {},
                                           DiagnosticSink& diagnostics = nullDiagnosticSink());
