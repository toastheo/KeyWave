#include "fallingnotes/FallingNotesSceneBuilder.hpp"

#include <cmath>
#include <iostream>
#include <iterator>
#include <vector>

#include "fallingnotes/FallingNotesLayout.hpp"
#include "fallingnotes/FallingNotesRenderAdapter.hpp"
#include "keyboard/KeyboardGeometry.hpp"
#include "keyboard/KeyboardLayout.hpp"
#include "keyboard/KeyboardRenderAdapter.hpp"
#include "keyboard/KeyboardState.hpp"
#include "midi/MidiTimelineQuery.hpp"

namespace {

RendererView rendererViewForKeyboard(const KeyboardGeometry& geometry,
                                     const FallingNotesViewport& viewport)
{
  const RendererView view{
    .visibleWorldRect =
      WorldRect{
        .x = 0.0,
        .y = -geometry.height(),
        .width = geometry.width(),
        .height = viewport.lookAheadSeconds + geometry.height(),
      },
  };

  if (!std::isfinite(viewport.currentTimeSeconds) || !isValid(view.visibleWorldRect)) {
    std::cerr << "Using default renderer view because the piano-roll view is invalid"
              << " (keyboardWidth=" << geometry.width() << ", keyboardHeight=" << geometry.height()
              << ", lookAheadSeconds=" << viewport.lookAheadSeconds << ").\n";
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

RenderScene FallingNotesSceneBuilder::build(const MidiTimeline& timeline,
                                            const double currentTimeSeconds,
                                            const FallingNotesSettings& fallingNotesSettings,
                                            const KeyboardSettings& keyboardSettings)
{
  const auto sanitizedFallingNotesSettings = sanitizeFallingNotesSettings(fallingNotesSettings);
  const auto keyboardConfig =
    keyboardLayoutConfigFromSettings(keyboardSettings, sanitizedFallingNotesSettings.pitchRange);
  const KeyboardGeometry keyboardGeometry(keyboardConfig);

  const FallingNotesViewport viewport{
    .pitchRange = sanitizedFallingNotesSettings.pitchRange,
    .currentTimeSeconds = currentTimeSeconds,
    .lookAheadSeconds = sanitizedFallingNotesSettings.lookAheadSeconds,
    .visiblePastSeconds = sanitizedFallingNotesSettings.visiblePastSeconds,
  };

  std::vector<QueriedNote> notes;
  std::vector<Note> activeNotes;
  if (std::isfinite(currentTimeSeconds)) {
    const MidiTimelineQuery query(timeline);
    notes = query.findNotes(TimelineViewport{
      .timeRange =
        TimeRange{
          .startSeconds = currentTimeSeconds - sanitizedFallingNotesSettings.visiblePastSeconds,
          .endSeconds = currentTimeSeconds + sanitizedFallingNotesSettings.lookAheadSeconds,
        },
      .pitchRange = sanitizedFallingNotesSettings.pitchRange,
    });
    activeNotes = query.findActiveNotesAt(currentTimeSeconds);
  }

  const auto fallingNotesLayout = FallingNotesLayout::build(notes, viewport, keyboardGeometry);
  const auto keyboardState = KeyboardStateBuilder::build(activeNotes);
  const auto keyboardLayout = KeyboardLayout::build(keyboardGeometry, keyboardState);

  std::vector<RenderCommand> commands;
  appendCommands(commands,
                 FallingNotesRenderAdapter::buildCommands(fallingNotesLayout,
                                                          fallingNotesRenderStyleFromSettings(
                                                            sanitizedFallingNotesSettings)));
  appendCommands(commands,
                 KeyboardRenderAdapter::buildCommands(
                   keyboardLayout, keyboardRenderStyleFromSettings(keyboardSettings)));

  return RenderScene{
    .commands = std::move(commands),
    .view = rendererViewForKeyboard(keyboardGeometry, viewport),
  };
}
