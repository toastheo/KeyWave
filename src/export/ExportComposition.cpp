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
                             ExportCompositionOptions options,
                             DiagnosticSink& diagnostics)
{
  const auto composition = exportComposition(config, size, options);
  auto scene =
    PianoRollSceneBuilder::build(timeline, sourceSeconds, composition.sceneConfig, diagnostics);
  scene.view = composition.view;
  return scene;
}

ExportComposition exportComposition(const PianoRollSceneConfig& config,
                                    FramebufferSize size,
                                    ExportCompositionOptions options)
{
  const auto source = options.sourceFramebufferSize;
  if (size.width <= 0 || size.height <= 0 || source.width <= 0 || source.height <= 0) {
    throw std::invalid_argument(
      "Export composition requires positive source and output dimensions.");
  }

  auto composed = config;
  composed.keyboardLayout.pitchRange = config.pitchRange;
  const KeyboardGeometry liveKeys(composed.keyboardLayout);
  composed.keyboardLayout.includeEdgeBlackKeys = true;
  const KeyboardGeometry exportKeys(composed.keyboardLayout);
  // A black-only selection has no usable live width; use its complete export
  // geometry as the reference. Other ranges retain their original live scale.
  const auto& referenceKeys = liveKeys.width() > 0 ? liveKeys : exportKeys;
  const double referenceHeight = config.displayHeight + referenceKeys.height();
  const double sourceAspect = static_cast<double>(source.width) / source.height;
  // The live view scales its two axes independently. Preserve the resulting
  // on-screen key proportions before fitting them into the output composition.
  const double verticalScale = referenceKeys.width() / (referenceHeight * sourceAspect);
  if (!std::isfinite(verticalScale) || verticalScale <= 0 || !std::isfinite(config.displayHeight) ||
      config.displayHeight <= 0) {
    throw std::invalid_argument("Export composition has invalid reference geometry.");
  }
  composed.keyboardLayout.whiteKeyHeight = exportKeys.config().whiteKeyHeight * verticalScale;
  composed.keyboardLayout.blackKeyHeight = exportKeys.config().blackKeyHeight * verticalScale;
  composed.keyboardStyle.hitLineHeight *= verticalScale;
  const KeyboardGeometry keys(composed.keyboardLayout);
  const double aspect = static_cast<double>(size.width) / size.height;
  // Retain explicitly tall keyboards. Wider output formats may add horizontal
  // margins rather than squeezing the configured key proportions or cropping keys.
  const double maximumKeyboardFraction = std::max(0.25, referenceKeys.height() / referenceHeight);
  const double width = std::max(keys.width(), keys.height() * aspect / maximumKeyboardFraction);
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
