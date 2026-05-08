#include "app/StartupSceneBuilder.hpp"

#include <filesystem>
#include <iostream>
#include <utility>

#include "midi/MidiFileLoader.hpp"

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

StartupData loadStartupMidiIfPresent(const AppConfig& config)
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

  auto timeline = MidiFileLoader::loadFromFile(midiPath);
  if (!timeline.has_value()) {
    std::cerr << "Warning: MIDI loading failed. Opening an empty window.\n";
    return {};
  }

  std::cout << "Loaded MIDI file with " << timeline->notes().size()
            << " note(s), length=" << timeline->lengthSeconds() << "s.\n";

  return StartupData{
    .timeline = std::move(timeline),
  };
}

} // namespace

StartupData StartupSceneBuilder::load(const AppConfig& config)
{
  return loadStartupMidiIfPresent(config);
}
