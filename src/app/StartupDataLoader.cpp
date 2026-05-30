#include <filesystem>
#include <sstream>
#include <utility>

#include "app/StartupDataLoader.hpp"
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
    reportInfo(diagnostics,
               "  resolved absolute path unavailable: " + errorCode.message());
    return;
  }

  reportInfo(diagnostics, "  resolved absolute path: " + absolutePath.string());
}

StartupData loadStartupMidiIfPresent(const AppConfig& config, DiagnosticSink& diagnostics)
{
  if (!config.midiFilePath.has_value()) {
    reportInfo(diagnostics, "No MIDI file provided. Starting with an empty window.");
    return {};
  }

  const auto& midiPath = *config.midiFilePath;
  reportInfo(diagnostics, "Loading MIDI file: " + midiPath.string());
  reportResolvedPathIfRelative(midiPath, diagnostics);

  std::error_code errorCode;
  if (!std::filesystem::exists(midiPath, errorCode)) {
    std::ostringstream message;
    message << "Warning: MIDI file does not exist: " << midiPath.string();
    if (errorCode) {
      message << " (" << errorCode.message() << ')';
    }
    message << ". Opening an empty window.";
    reportWarning(diagnostics, message.str());
    return {};
  }

  if (!std::filesystem::is_regular_file(midiPath, errorCode)) {
    std::ostringstream message;
    message << "Warning: MIDI path is not a regular file: " << midiPath.string();
    if (errorCode) {
      message << " (" << errorCode.message() << ')';
    }
    message << ". Opening an empty window.";
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

} // namespace

StartupData StartupDataLoader::load(const AppConfig& config, DiagnosticSink& diagnostics)
{
  return loadStartupMidiIfPresent(config, diagnostics);
}
