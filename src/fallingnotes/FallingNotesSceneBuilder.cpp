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
#include "midi/MidiTimelineQuery.hpp"

namespace {

constexpr PitchRange kPianoPitchRange{
  .minPitch = 21,
  .maxPitch = 108,
};

constexpr double kLookAheadSeconds = 10.0;
constexpr double kVisiblePastSeconds = 0.0;

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
                                            const double currentTimeSeconds)
{
  constexpr KeyboardLayoutConfig keyboardConfig{
    .pitchRange = kPianoPitchRange,
  };
  const KeyboardGeometry keyboardGeometry(keyboardConfig);

  const FallingNotesViewport viewport{
    .pitchRange = kPianoPitchRange,
    .currentTimeSeconds = currentTimeSeconds,
    .lookAheadSeconds = kLookAheadSeconds,
    .visiblePastSeconds = kVisiblePastSeconds,
  };

  std::vector<QueriedNote> notes;
  if (std::isfinite(currentTimeSeconds)) {
    const MidiTimelineQuery query(timeline);
    notes = query.findNotes(TimelineViewport{
      .timeRange =
        TimeRange{
          .startSeconds = currentTimeSeconds - kVisiblePastSeconds,
          .endSeconds = currentTimeSeconds + kLookAheadSeconds,
        },
      .pitchRange = kPianoPitchRange,
    });
  }

  const auto fallingNotesLayout = FallingNotesLayout::build(notes, viewport, keyboardGeometry);
  const auto keyboardLayout = KeyboardLayout::build(keyboardGeometry);

  std::vector<RenderCommand> commands;
  appendCommands(commands, FallingNotesRenderAdapter::buildCommands(fallingNotesLayout));
  appendCommands(commands, KeyboardRenderAdapter::buildCommands(keyboardLayout));

  return RenderScene{
    .commands = std::move(commands),
    .view = rendererViewForKeyboard(keyboardGeometry, viewport),
  };
}
