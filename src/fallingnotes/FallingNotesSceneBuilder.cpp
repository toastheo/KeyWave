#include "fallingnotes/FallingNotesSceneBuilder.hpp"

#include <iostream>

#include "fallingnotes/FallingNotesLayout.hpp"
#include "fallingnotes/FallingNotesRenderAdapter.hpp"
#include "midi/MidiTimelineQuery.hpp"

namespace {

constexpr PitchRange kPianoPitchRange{
  .minPitch = 21,
  .maxPitch = 108,
};

constexpr double kLookAheadSeconds = 10.0;
constexpr double kVisiblePastSeconds = 0.0;

RendererView rendererViewForLayout(const FallingNotesLayoutResult& layoutResult)
{
  const RendererView view{
    .visibleWorldRect =
      WorldRect{
        .x = 0.0,
        .y = 0.0,
        .width = layoutResult.contentWidth,
        .height = layoutResult.contentHeight,
      },
  };

  if (!isValid(view.visibleWorldRect)) {
    std::cerr << "Using default renderer view because the falling-notes layout size is invalid"
              << " (contentWidth=" << layoutResult.contentWidth
              << ", contentHeight=" << layoutResult.contentHeight << ").\n";
    return RendererView{};
  }

  return view;
}

} // namespace

RenderScene FallingNotesSceneBuilder::build(const MidiTimeline& timeline,
                                            const double currentTimeSeconds)
{
  const FallingNotesViewport viewport{
    .pitchRange = kPianoPitchRange,
    .currentTimeSeconds = currentTimeSeconds,
    .lookAheadSeconds = kLookAheadSeconds,
    .visiblePastSeconds = kVisiblePastSeconds,
  };

  const MidiTimelineQuery query(timeline);
  const auto notes = query.findNotes(TimelineViewport{
    .timeRange =
      TimeRange{
        .startSeconds = currentTimeSeconds - kVisiblePastSeconds,
        .endSeconds = currentTimeSeconds + kLookAheadSeconds,
      },
    .pitchRange = kPianoPitchRange,
  });

  const auto layoutResult = FallingNotesLayout::build(notes, viewport);
  return RenderScene{
    .commands = FallingNotesRenderAdapter::buildCommands(layoutResult),
    .view = rendererViewForLayout(layoutResult),
  };
}
