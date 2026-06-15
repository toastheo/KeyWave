#include <catch2/catch_test_macros.hpp>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

#include "app/MidiLibraryStore.hpp"

namespace {

std::filesystem::path uniqueLibraryRoot()
{
  const auto suffix = std::chrono::steady_clock::now().time_since_epoch().count();
  return std::filesystem::temp_directory_path() /
         ("keywave-midi-library-test-" + std::to_string(suffix));
}

std::filesystem::path writeSourceMidi(const std::filesystem::path& directory,
                                      const std::string& fileName,
                                      const std::vector<unsigned char>& bytes)
{
  std::filesystem::create_directories(directory);
  const auto path = directory / fileName;
  std::ofstream output(path, std::ios::binary | std::ios::trunc);
  for (const auto byte : bytes) {
    output.put(static_cast<char>(byte));
  }
  return path;
}

std::vector<unsigned char> readBytes(const std::filesystem::path& path)
{
  std::ifstream input(path, std::ios::binary);
  return {std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
}

TEST_CASE("MidiLibraryStore imports a MIDI file into app-owned storage", "[app][midi-library]")
{
  const auto root = uniqueLibraryRoot();
  const auto sourcePath =
    writeSourceMidi(root / "source", "song.mid", {'M', 'T', 'h', 'd', 0, 1, 2, 3});
  MidiLibraryStore const store(root / "library");

  const auto result = store.importFile(sourcePath);

  REQUIRE(result.has_value());
  CHECK_FALSE(result->alreadyImported);
  CHECK_FALSE(result->file.id.empty());
  CHECK(result->file.displayName == "song");
  CHECK(result->file.originalFileName == "song.mid");
  CHECK(result->file.storedFileName == result->file.id + ".mid");
  CHECK_FALSE(result->file.contentHash.empty());
  CHECK(result->file.sizeBytes == 8);
  CHECK_FALSE(result->file.importedAt.empty());
  CHECK(result->file.lastOpenedAt == result->file.importedAt);

  const auto storedPath = store.importedFilePath(result->file.id);
  REQUIRE(storedPath.has_value());
  CHECK(storedPath->parent_path().filename() == "files");
  CHECK(readBytes(*storedPath) == readBytes(sourcePath));
  CHECK(std::filesystem::exists(root / "library" / "midi-library.json"));
}

TEST_CASE("MidiLibraryStore persists imported MIDI metadata", "[app][midi-library]")
{
  const auto root = uniqueLibraryRoot();
  const auto libraryRoot = root / "library";
  const auto sourcePath =
    writeSourceMidi(root / "source", "persisted.midi", {'M', 'T', 'h', 'd', 0, 1, 2, 3});
  MidiLibraryStore const store(libraryRoot);
  const auto imported = store.importFile(sourcePath);
  REQUIRE(imported.has_value());

  MidiLibraryStore const reopenedStore(libraryRoot);
  const auto files = reopenedStore.listImportedFiles();

  REQUIRE(files.size() == 1);
  CHECK(files.front().id == imported->file.id);
  CHECK(files.front().displayName == "persisted");
  CHECK(files.front().storedFileName == imported->file.storedFileName);

  std::ifstream metadataInput(libraryRoot / "midi-library.json");
  const auto metadata = nlohmann::json::parse(metadataInput);
  REQUIRE(metadata.at("files").is_array());
  CHECK(metadata.at("files").size() == 1);
}

TEST_CASE("MidiLibraryStore reuses an existing import for identical bytes", "[app][midi-library]")
{
  const auto root = uniqueLibraryRoot();
  const std::vector<unsigned char> bytes{'M', 'T', 'h', 'd', 8, 9, 10, 11};
  const auto firstSource = writeSourceMidi(root / "source-a", "first.mid", bytes);
  const auto secondSource = writeSourceMidi(root / "source-b", "renamed.mid", bytes);
  MidiLibraryStore const store(root / "library");

  const auto firstImport = store.importFile(firstSource);
  REQUIRE(firstImport.has_value());
  const auto secondImport = store.importFile(secondSource);

  REQUIRE(secondImport.has_value());
  CHECK(secondImport->alreadyImported);
  CHECK(secondImport->file.id == firstImport->file.id);
  CHECK(store.listImportedFiles().size() == 1);
}

TEST_CASE("MidiLibraryStore persists the last active imported MIDI id", "[app][midi-library]")
{
  const auto root = uniqueLibraryRoot();
  const auto sourcePath =
    writeSourceMidi(root / "source", "stored.mid", {'M', 'T', 'h', 'd', 16, 17, 18, 19});
  const auto libraryRoot = root / "library";
  MidiLibraryStore const store(libraryRoot);
  const auto imported = store.importFile(sourcePath);
  REQUIRE(imported.has_value());

  CHECK(store.setLastActiveMidiId(imported->file.id));

  MidiLibraryStore const reopenedStore(libraryRoot);
  const auto lastActiveId = reopenedStore.lastActiveMidiId();
  REQUIRE(lastActiveId.has_value());
  CHECK(*lastActiveId == imported->file.id);
}

TEST_CASE("MidiLibraryStore rejects unknown last active imported MIDI ids", "[app][midi-library]")
{
  const auto root = uniqueLibraryRoot();
  const auto sourcePath =
    writeSourceMidi(root / "source", "stored.mid", {'M', 'T', 'h', 'd', 20, 21, 22, 23});
  MidiLibraryStore const store(root / "library");
  REQUIRE(store.importFile(sourcePath).has_value());

  CHECK_FALSE(store.setLastActiveMidiId("missing-id"));
  CHECK_FALSE(store.lastActiveMidiId().has_value());
}

TEST_CASE("MidiLibraryStore renames imported MIDI display names without renaming stored files",
          "[app][midi-library]")
{
  const auto root = uniqueLibraryRoot();
  const auto libraryRoot = root / "library";
  const auto sourcePath =
    writeSourceMidi(root / "source", "stored.mid", {'M', 'T', 'h', 'd', 24, 25, 26, 27});
  MidiLibraryStore const store(libraryRoot);
  const auto imported = store.importFile(sourcePath);
  REQUIRE(imported.has_value());

  CHECK(store.renameImportedMidiFile(imported->file.id, "  Better Name  "));

  MidiLibraryStore const reopenedStore(libraryRoot);
  const auto renamed = reopenedStore.findById(imported->file.id);
  REQUIRE(renamed.has_value());
  CHECK(renamed->displayName == "Better Name");
  CHECK(renamed->storedFileName == imported->file.storedFileName);
  CHECK(reopenedStore.importedFilePath(imported->file.id).has_value());
}

TEST_CASE("MidiLibraryStore rejects invalid imported MIDI renames", "[app][midi-library]")
{
  const auto root = uniqueLibraryRoot();
  const auto sourcePath =
    writeSourceMidi(root / "source", "stored.mid", {'M', 'T', 'h', 'd', 28, 29, 30, 31});
  MidiLibraryStore const store(root / "library");
  const auto imported = store.importFile(sourcePath);
  REQUIRE(imported.has_value());

  CHECK_FALSE(store.renameImportedMidiFile("missing-id", "Other Name"));
  CHECK_FALSE(store.renameImportedMidiFile(imported->file.id, "    "));

  const auto unchanged = store.findById(imported->file.id);
  REQUIRE(unchanged.has_value());
  CHECK(unchanged->displayName == imported->file.displayName);
}

TEST_CASE("MidiLibraryStore removes imported MIDI files and their copied files",
          "[app][midi-library]")
{
  const auto root = uniqueLibraryRoot();
  const auto sourcePath =
    writeSourceMidi(root / "source", "stored.mid", {'M', 'T', 'h', 'd', 32, 33, 34, 35});
  MidiLibraryStore const store(root / "library");
  const auto imported = store.importFile(sourcePath);
  REQUIRE(imported.has_value());
  REQUIRE(store.setLastActiveMidiId(imported->file.id));
  const auto copiedPath = store.importedFilePath(imported->file.id);
  REQUIRE(copiedPath.has_value());

  CHECK(store.removeImportedMidiFile(imported->file.id));

  CHECK_FALSE(std::filesystem::exists(*copiedPath));
  CHECK_FALSE(store.findById(imported->file.id).has_value());
  CHECK_FALSE(store.lastActiveMidiId().has_value());
}

TEST_CASE("MidiLibraryStore rejects unknown imported MIDI removals", "[app][midi-library]")
{
  const auto root = uniqueLibraryRoot();
  const auto sourcePath =
    writeSourceMidi(root / "source", "stored.mid", {'M', 'T', 'h', 'd', 36, 37, 38, 39});
  MidiLibraryStore const store(root / "library");
  const auto imported = store.importFile(sourcePath);
  REQUIRE(imported.has_value());

  CHECK_FALSE(store.removeImportedMidiFile("missing-id"));
  CHECK(store.findById(imported->file.id).has_value());
}

TEST_CASE("MidiLibraryStore returns stored paths only for existing copied files",
          "[app][midi-library]")
{
  const auto root = uniqueLibraryRoot();
  const auto sourcePath =
    writeSourceMidi(root / "source", "stored.mid", {'M', 'T', 'h', 'd', 12, 13, 14, 15});
  MidiLibraryStore const store(root / "library");
  const auto imported = store.importFile(sourcePath);
  REQUIRE(imported.has_value());

  const auto storedPath = store.importedFilePath(imported->file.id);
  REQUIRE(storedPath.has_value());
  std::filesystem::remove(*storedPath);

  CHECK_FALSE(store.importedFilePath(imported->file.id).has_value());
  CHECK_FALSE(store.importedFilePath("missing-id").has_value());
}

TEST_CASE("MidiLibraryStore rejects missing source files without writing metadata",
          "[app][midi-library]")
{
  const auto root = uniqueLibraryRoot();
  MidiLibraryStore const store(root / "library");

  const auto result = store.importFile(root / "source" / "missing.id");

  CHECK_FALSE(result.has_value());
  CHECK(store.listImportedFiles().empty());
  CHECK_FALSE(std::filesystem::exists(root / "library" / "midi-library.json"));
}

} // namespace
