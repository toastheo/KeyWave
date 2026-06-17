#include <catch2/catch_test_macros.hpp>
#include <chrono>
#include <filesystem>
#include <string>

#include "app/AppConfig.hpp"
#include "app/MidiLibraryStore.hpp"
#include "app/StartupDataLoader.hpp"
#include "midi/MidiFixtures.hpp"

namespace {

std::filesystem::path uniqueStartupLibraryRoot()
{
  const auto suffix = std::chrono::steady_clock::now().time_since_epoch().count();
  return std::filesystem::temp_directory_path() /
         ("keywave-startup-midi-library-test-" + std::to_string(suffix));
}

TEST_CASE("StartupDataLoader loads the last active imported MIDI when no CLI path is provided",
          "[app][startup][midi-library]")
{
  auto fixture = midi_fixtures::tempoChangeMidi();
  const auto root = uniqueStartupLibraryRoot();
  MidiLibraryStore const store(root);
  const auto imported = store.importFile(fixture.path());
  REQUIRE(imported.has_value());
  REQUIRE(store.setLastActiveMidiId(imported->file.id));

  const auto startupData = StartupDataLoader::load(AppConfig{}, store);

  REQUIRE(startupData.timeline.has_value());
  CHECK(startupData.timeline->notes().size() == 2);
  REQUIRE(startupData.importedMidiId.has_value());
  CHECK(*startupData.importedMidiId == imported->file.id);
}

TEST_CASE("StartupDataLoader keeps CLI MIDI path precedence over imported MIDI restore",
          "[app][startup][midi-library]")
{
  auto fixture = midi_fixtures::tempoChangeMidi();
  const auto root = uniqueStartupLibraryRoot();
  MidiLibraryStore const store(root);
  const auto imported = store.importFile(fixture.path());
  REQUIRE(imported.has_value());
  REQUIRE(store.setLastActiveMidiId(imported->file.id));

  const AppConfig config{.midiFilePath = root / "missing-cli-file.mid"};
  const auto startupData = StartupDataLoader::load(config, store);

  CHECK_FALSE(startupData.timeline.has_value());
  CHECK_FALSE(startupData.importedMidiId.has_value());
}

TEST_CASE("StartupDataLoader starts empty when the last active imported copy is unavailable",
          "[app][startup][midi-library]")
{
  auto fixture = midi_fixtures::tempoChangeMidi();
  MidiLibraryStore const store(uniqueStartupLibraryRoot());
  const auto imported = store.importFile(fixture.path());
  REQUIRE(imported.has_value());
  REQUIRE(store.setLastActiveMidiId(imported->file.id));
  const auto importedPath = store.importedFilePath(imported->file.id);
  REQUIRE(importedPath.has_value());
  std::filesystem::remove(*importedPath);

  const auto startupData = StartupDataLoader::load(AppConfig{}, store);

  CHECK_FALSE(startupData.timeline.has_value());
  CHECK_FALSE(startupData.importedMidiId.has_value());
}

} // namespace
