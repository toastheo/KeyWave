#include "fallingnotes/PianoRollSceneBuilder.hpp"

#include <cmath>
#include <iterator>
#include <sstream>
#include <utility>
#include <vector>

#include "core/CoreTypes.hpp"
#include "diagnostics/Diagnostics.hpp"
#include "fallingnotes/FallingNotesLayout.hpp"
#include "fallingnotes/FallingNotesRenderAdapter.hpp"
#include "fallingnotes/FallingNotesTypes.hpp"
#include "keyboard/KeyboardGeometry.hpp"
#include "keyboard/KeyboardLayout.hpp"
#include "keyboard/KeyboardRenderAdapter.hpp"
#include "keyboard/KeyboardState.hpp"
#include "midi/MidiTimeline.hpp"
#include "midi/MidiTimelineQuery.hpp"
#include "midi/MidiTypes.hpp"
#include "render/RenderCommand.hpp"
#include "render/RenderScene.hpp"
#include "render/RendererView.hpp"

namespace {

bool isValidPitchRange(const PitchRange& range)
{
  return range.minPitch <= range.maxPitch;
}

bool isNonNegativeFinite(const double value)
{
  return std::isfinite(value) && value >= 0.0;
}

bool isPositiveFinite(const double value)
{
  return std::isfinite(value) && value > 0.0;
}

PianoRollSceneConfig sanitizedConfig(PianoRollSceneConfig config)
{
  constexpr PianoRollSceneConfig defaults;

  if (!isValidPitchRange(config.pitchRange)) {
    config.pitchRange = defaults.pitchRange;
  }
  if (!isPositiveFinite(config.lookAheadSeconds)) {
    config.lookAheadSeconds = defaults.lookAheadSeconds;
  }
  if (!isNonNegativeFinite(config.visiblePastSeconds)) {
    config.visiblePastSeconds = defaults.visiblePastSeconds;
  }
  if (!isPositiveFinite(config.displayHeight)) {
    config.displayHeight = defaults.displayHeight;
  }
  config.keyboardLayout.pitchRange = config.pitchRange;

  return config;
}

RendererView rendererViewForKeyboard(const KeyboardGeometry& geometry,
                                     const FallingNotesViewport& viewport,
                                     DiagnosticSink& diagnostics)
{
  const auto fallingNotesHeight = viewport.displayHeight > 0.0 ? viewport.displayHeight
                                                               : viewport.lookAheadSeconds;
  const RendererView view{
    .visibleWorldRect =
      WorldRect{
        .x = 0.0,
        .y = -geometry.height(),
        .width = geometry.width(),
        .height = fallingNotesHeight + geometry.height(),
      },
  };

  if (!std::isfinite(viewport.currentTimeSeconds) || !isValid(view.visibleWorldRect)) {
    std::ostringstream message;
    message << "Using default renderer view because the piano-roll view is invalid"
            << " (keyboardWidth=" << geometry.width() << ", keyboardHeight=" << geometry.height()
            << ", lookAheadSeconds=" << viewport.lookAheadSeconds << ").";
    reportWarning(diagnostics, message.str());
    return RendererView{};
  }

  return view;
}

void appendCommands(std::vector<RenderCommand>& destination, std::vector<RenderCommand> source)
{
  destination.insert(destination.end(),
                     std::make_move_iterator(source.begin()),
                     std::make_move_iterator(source.end()));
}

} // namespace

RenderScene PianoRollSceneBuilder::build(const MidiTimeline& timeline,
                                         const double currentTimeSeconds,
                                         const PianoRollSceneConfig& config,
                                         DiagnosticSink& diagnostics)
{
  const auto sanitized = sanitizedConfig(config);
  const KeyboardGeometry keyboardGeometry(sanitized.keyboardLayout);

  const FallingNotesViewport viewport{
    .pitchRange = sanitized.pitchRange,
    .currentTimeSeconds = currentTimeSeconds,
    .lookAheadSeconds = sanitized.lookAheadSeconds,
    .visiblePastSeconds = sanitized.visiblePastSeconds,
    .displayHeight = sanitized.displayHeight,
  };

  std::vector<QueriedNote> notes;
  std::vector<Note> activeNotes;
  if (std::isfinite(currentTimeSeconds)) {
    const MidiTimelineQuery query(timeline, diagnostics);
    notes = query.findNotes(TimelineViewport{
      .timeRange =
        TimeRange{
          .startSeconds = currentTimeSeconds - sanitized.visiblePastSeconds,
          .endSeconds = currentTimeSeconds + sanitized.lookAheadSeconds,
        },
      .pitchRange = sanitized.pitchRange,
    });
    activeNotes = query.findActiveNotesAt(currentTimeSeconds);
  }

  const auto fallingNotesLayout =
    FallingNotesLayout::build(notes, viewport, keyboardGeometry, sanitized.fallingNotesLayout);
  const auto keyboardState = KeyboardStateBuilder::build(activeNotes);
  const auto keyboardLayout = KeyboardLayout::build(keyboardGeometry, keyboardState);

  std::vector<RenderCommand> commands;
  appendCommands(commands,
                 FallingNotesRenderAdapter::buildCommands(fallingNotesLayout,
                                                          sanitized.fallingNotesStyle));
  appendCommands(commands,
                 KeyboardRenderAdapter::buildCommands(keyboardLayout, sanitized.keyboardStyle));

  return RenderScene{
    .commands = std::move(commands),
    .view = rendererViewForKeyboard(keyboardGeometry, viewport, diagnostics),
  };
}
