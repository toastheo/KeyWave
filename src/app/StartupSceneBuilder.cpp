#include "app/StartupSceneBuilder.hpp"

#include <filesystem>
#include <iostream>

#include "fallingnotes/FallingNotesLayout.hpp"
#include "fallingnotes/FallingNotesRenderAdapter.hpp"
#include "midi/MidiFileLoader.hpp"
#include "midi/MidiTimelineQuery.hpp"

namespace {

void printResolvedPathIfRelative(const std::filesystem::path& path)
{
  if (!path.is_relative()) {
    return;
  }

  std::error_code errorCode;
  const auto absolutePath = std::filesystem::absolute(path, errorCode);
  if (errorCode) {
    std::cout << "  resolved absolute path unavailable: " << errorCode.message() << '\n';
    return;
  }

  std::cout << "  resolved absolute path: " << absolutePath.string() << '\n';
}

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

RenderScene loadStartupMidiIfPresent(const AppConfig& config)
{
  if (!config.midiFilePath.has_value()) {
    std::cout << "No MIDI file provided. Starting with an empty window.\n";
    return {};
  }

  const auto& midiPath = *config.midiFilePath;
  std::cout << "Loading MIDI file: " << midiPath.string() << '\n';
  printResolvedPathIfRelative(midiPath);

  std::error_code errorCode;
  if (!std::filesystem::exists(midiPath, errorCode)) {
    std::cerr << "Warning: MIDI file does not exist: " << midiPath.string();
    if (errorCode) {
      std::cerr << " (" << errorCode.message() << ')';
    }
    std::cerr << ". Opening an empty window.\n";
    return {};
  }

  if (!std::filesystem::is_regular_file(midiPath, errorCode)) {
    std::cerr << "Warning: MIDI path is not a regular file: " << midiPath.string();
    if (errorCode) {
      std::cerr << " (" << errorCode.message() << ')';
    }
    std::cerr << ". Opening an empty window.\n";
    return {};
  }

  const auto timeline = MidiFileLoader::loadFromFile(midiPath);
  if (!timeline.has_value()) {
    std::cerr << "Warning: MIDI loading failed. Opening an empty window.\n";
    return {};
  }

  const MidiTimelineQuery query(*timeline);
  constexpr auto viewport = FallingNotesViewport{
    .pitchRange = PitchRange{.minPitch = 21, .maxPitch = 108},
    .currentTimeSeconds = 0.0,
    .lookAheadSeconds = 10.0,
    .visiblePastSeconds = 0.0,
  };
  const auto notes = query.findNotes(TimelineViewport{
    .timeRange =
      TimeRange{
        .startSeconds = viewport.currentTimeSeconds - viewport.visiblePastSeconds,
        .endSeconds = viewport.currentTimeSeconds + viewport.lookAheadSeconds,
      },
    .pitchRange = viewport.pitchRange,
  });

  const auto layoutResult = FallingNotesLayout::build(notes, viewport);
  const auto renderCommands = FallingNotesRenderAdapter::buildCommands(layoutResult);
  const auto rendererView = rendererViewForLayout(layoutResult);

  return RenderScene{
    .commands = renderCommands,
    .view = rendererView,
  };
}

} // namespace

RenderScene StartupSceneBuilder::build(const AppConfig& config)
{
  return loadStartupMidiIfPresent(config);
}
