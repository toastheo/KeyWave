#include "app/StartupDataLoader.hpp"

#include <chrono>
#include <filesystem>
#include <sstream>
#include <utility>

#include "midi/MidiFileLoader.hpp"

namespace {

void reportResolvedPathIfRelative(const std::filesystem::path& path, DiagnosticSink& diagnostics)
{
  if (!path.is_relative()) {
    return;
  }

  std::error_code errorCode;
  const auto absolutePath = std::filesystem::absolute(path, errorCode);
  if (errorCode) {
    reportInfo(diagnostics, "  resolved absolute path unavailable: " + errorCode.message());
    return;
  }

  reportInfo(diagnostics, "  resolved absolute path: " + absolutePath.string());
}

StartupData loadMidiFromPath(const std::filesystem::path& midiPath,
                             const char* missingFileContext,
                             DiagnosticSink& diagnostics)
{
  reportInfo(diagnostics, "Loading MIDI file: " + midiPath.string());
  reportResolvedPathIfRelative(midiPath, diagnostics);

  std::error_code errorCode;
  if (!std::filesystem::exists(midiPath, errorCode)) {
    std::ostringstream message;
    message << "Warning: MIDI file does not exist: " << midiPath.string();
    if (errorCode) {
      message << " (" << errorCode.message() << ')';
    }
    message << missingFileContext;
    reportWarning(diagnostics, message.str());
    return {};
  }

  if (!std::filesystem::is_regular_file(midiPath, errorCode)) {
    std::ostringstream message;
    message << "Warning: MIDI path is not a regular file: " << midiPath.string();
    if (errorCode) {
      message << " (" << errorCode.message() << ')';
    }
    message << missingFileContext;
    reportWarning(diagnostics, message.str());
    return {};
  }

  auto timeline = MidiFileLoader::loadFromFile(midiPath, diagnostics);
  if (!timeline.has_value()) {
    reportWarning(diagnostics, "Warning: MIDI loading failed. Opening an empty window.");
    return {};
  }

  std::ostringstream message;
  message << "Loaded MIDI file with " << timeline->notes().size()
          << " note(s), length=" << timeline->lengthSeconds() << "s.";
  reportInfo(diagnostics, message.str());

  return StartupData{
    .timeline = std::move(timeline),
  };
}

StartupData loadLastActiveImportedMidi(const MidiLibraryStore& midiLibraryStore,
                                       DiagnosticSink& diagnostics)
{
  const auto lastActiveId = midiLibraryStore.lastActiveMidiId(diagnostics);
  if (!lastActiveId.has_value()) {
    reportInfo(diagnostics,
               "No last active imported MIDI file found. Starting with an empty window.");
    return {};
  }

  const auto midiPath = midiLibraryStore.importedFilePath(*lastActiveId, diagnostics);
  if (!midiPath.has_value()) {
    reportWarning(
      diagnostics,
      "Warning: last active imported MIDI file is unavailable. Opening an empty window.");
    return {};
  }

  reportInfo(diagnostics, "Loading last active imported MIDI file: " + *lastActiveId);
  auto startupData = loadMidiFromPath(*midiPath, ". Opening an empty window.", diagnostics);
  if (startupData.timeline.has_value()) {
    startupData.importedMidiId = *lastActiveId;
  }
  return startupData;
}

StartupData loadStartupMidiIfPresent(const AppConfig& config,
                                     const MidiLibraryStore& midiLibraryStore,
                                     DiagnosticSink& diagnostics)
{
  if (!config.midiFilePath.has_value()) {
    return loadLastActiveImportedMidi(midiLibraryStore, diagnostics);
  }

  return loadMidiFromPath(*config.midiFilePath, ". Opening an empty window.", diagnostics);
}

} // namespace

StartupData StartupDataLoader::load(const AppConfig& config,
                                    const MidiLibraryStore& midiLibraryStore,
                                    DiagnosticSink& diagnostics)
{
  return loadStartupMidiIfPresent(config, midiLibraryStore, diagnostics);
}
