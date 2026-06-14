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