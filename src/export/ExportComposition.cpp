#include "export/ExportComposition.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>

#include "core/CoreTypes.hpp"
#include "diagnostics/Diagnostics.hpp"
#include "fallingnotes/PianoRollSceneBuilder.hpp"
#include "keyboard/KeyboardGeometry.hpp"
#include "midi/MidiTimeline.hpp"
#include "render/RenderScene.hpp"

RenderScene buildExportScene(const MidiTimeline& timeline,
                             double sourceSeconds,
                             const PianoRollSceneConfig& config,
                             FramebufferSize size,
                             DiagnosticSink& diagnostics)
{
  const auto composition = exportComposition(config, size);
  auto scene =
    PianoRollSceneBuilder::build(timeline, sourceSeconds, composition.sceneConfig, diagnostics);
  scene.view = composition.view;
  return scene;
}

ExportComposition exportComposition(const PianoRollSceneConfig& config, FramebufferSize size)
{
  if (size.width <= 0 || size.height <= 0) {
    throw std::invalid_argument("Export composition requires positive dimensions.");
  }

  auto composed = config;
  composed.keyboardLayout.pitchRange = config.pitchRange;
  composed.keyboardLayout.includeEdgeBlackKeys = true;
  const KeyboardGeometry keys(composed.keyboardLayout);
  const double aspect = static_cast<double>(size.width) / size.height;
  // Leave at least 75% of the height for notes even for a very narrow pitch range.
  constexpr double maximumKeyboardFriction = 0.25;
  const double width = std::max(keys.width(), keys.height() * aspect / maximumKeyboardFriction);
  const double height = width / aspect;
  if (!std::isfinite(width) || width <= 0 || !std::isfinite(height) || height <= 0) {
    throw std::invalid_argument("Export composition has invalid keyboard geometry.");
  }

  composed.displayHeight = height - keys.height();
  constexpr double referenceShortSide = 1080.0;
  const double scale = std::min(size.width, size.height) / referenceShortSide;
  composed.fallingNotesStyle.outlineThicknessPixels *= scale;
  composed.fallingNotesStyle.cornerRadiusPixels *= scale;
  composed.keyboardStyle.separatorThicknessPixels *= scale;
  return {.sceneConfig = composed,
          .view = {.visibleWorldRect = {.x = (keys.width() - width) * 0.5,
                                        .y = -keys.height(),
                                        .width = width,
                                        .height = height}}};
}
