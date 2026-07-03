#include <algorithm>
#include <catch2/catch_test_macros.hpp>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <nlohmann/json.hpp>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "app/MidiLibraryStore.hpp"
#include "diagnostics/RecordingDiagnosticSink.hpp"

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

void writeText(const std::filesystem::path& path, const std::string& text)
{
  std::filesystem::create_directories(path.parent_path());
  std::ofstream output(path, std::ios::trunc);
  output << text;
}

nlohmann::json midiFileMetadata(std::string id,
                                std::string storedFileName,
                                const std::uintmax_t sizeBytes,
                                std::string contentHash = "0123456789abcdef",
                                std::string importedAt = "2026-06-17T12:34:56Z",
                                std::string lastOpenedAt = "2026-06-17T12:34:56Z")
{
  return nlohmann::json{
    {"id", std::move(id)},
    {"displayName", "Stored song"},
    {"storedFileName", std::move(storedFileName)},
    {"originalFileName", "stored.mid"},
    {"contentHash", std::move(contentHash)},
    {"sizeBytes", sizeBytes},
    {"importedAt", std::move(importedAt)},
    {"lastOpenedAt", std::move(lastOpenedAt)},
  };
}

bool hasDiagnosticContaining(const RecordingDiagnosticSink& diagnostics,
                             const std::string_view text)
{
  return std::ranges::any_of(diagnostics.messages, [text](const RecordedDiagnostic& diagnostic) {
    return diagnostic.message.find(text) != std::string::npos;
  });
}

TEST_CASE("MidiLibraryStore imports a MIDI file into app-owned storage", "[app][midi-library]")
{
  const auto root = uniqueLibraryRoot();
  const auto sourcePath =
    writeSourceMidi(root / "source", "song.mid", {'M', 'T', 'h', 'd', 0, 1, 2, 3});
  MidiLibraryStore store(root / "library");

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
  MidiLibraryStore store(libraryRoot);
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
  MidiLibraryStore store(root / "library");

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
  MidiLibraryStore store(libraryRoot);
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
  MidiLibraryStore store(root / "library");
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
  MidiLibraryStore store(libraryRoot);
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
  MidiLibraryStore store(root / "library");
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
  MidiLibraryStore store(root / "library");
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

TEST_CASE("MidiLibraryStore keeps copied files when removal metadata save fails",
          "[app][midi-library]")
{
  const auto root = uniqueLibraryRoot();
  const auto libraryRoot = root / "library";
  const auto sourcePath =
    writeSourceMidi(root / "source", "stored.mid", {'M', 'T', 'h', 'd', 48, 49, 50, 51});
  MidiLibraryStore store(libraryRoot);
  const auto imported = store.importFile(sourcePath);
  REQUIRE(imported.has_value());
  REQUIRE(store.setLastActiveMidiId(imported->file.id));
  const auto copiedPath = store.importedFilePath(imported->file.id);
  REQUIRE(copiedPath.has_value());

  std::filesystem::create_directory(libraryRoot / "midi-library.json.tmp");

  CHECK_FALSE(store.removeImportedMidiFile(imported->file.id));

  CHECK(std::filesystem::exists(*copiedPath));
  CHECK(store.findById(imported->file.id).has_value());
  const auto lastActiveId = store.lastActiveMidiId();
  REQUIRE(lastActiveId.has_value());
  CHECK(*lastActiveId == imported->file.id);
}

TEST_CASE("MidiLibraryStore removes metadata when copied file cleanup fails", "[app][midi-library]")
{
  const auto root = uniqueLibraryRoot();
  const auto sourcePath =
    writeSourceMidi(root / "source", "stored.mid", {'M', 'T', 'h', 'd', 52, 53, 54, 55});
  MidiLibraryStore store(root / "library");
  const auto imported = store.importFile(sourcePath);
  REQUIRE(imported.has_value());
  const auto copiedPath = store.importedFilePath(imported->file.id);
  REQUIRE(copiedPath.has_value());
  std::filesystem::remove(*copiedPath);
  std::filesystem::create_directory(*copiedPath);
  writeText(*copiedPath / "nested-file", "still here");
  RecordingDiagnosticSink diagnostics;

  CHECK(store.removeImportedMidiFile(imported->file.id, diagnostics));

  CHECK_FALSE(store.findById(imported->file.id).has_value());
  CHECK(std::filesystem::exists(*copiedPath));
  REQUIRE_FALSE(diagnostics.messages.empty());
  CHECK(diagnostics.messages.back().message.find("could not delete copied MIDI file") !=
        std::string::npos);
}

TEST_CASE("MidiLibraryStore rejects unknown imported MIDI removals", "[app][midi-library]")
{
  const auto root = uniqueLibraryRoot();
  const auto sourcePath =
    writeSourceMidi(root / "source", "stored.mid", {'M', 'T', 'h', 'd', 36, 37, 38, 39});
  MidiLibraryStore store(root / "library");
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
  MidiLibraryStore store(root / "library");
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
  MidiLibraryStore store(root / "library");

  const auto result = store.importFile(root / "source" / "missing.id");

  CHECK_FALSE(result.has_value());
  CHECK(store.listImportedFiles().empty());
  CHECK_FALSE(std::filesystem::exists(root / "library" / "midi-library.json"));
}

TEST_CASE("MidiLibraryStore keeps malformed metadata read-only", "[app][midi-library]")
{
  const auto root = uniqueLibraryRoot();
  const auto metadataPath = root / "library" / "midi-library.json";
  writeText(metadataPath, "{ invalid json");
  MidiLibraryStore const store(root / "library");

  CHECK(store.listImportedFiles().empty());
  CHECK_FALSE(store.lastActiveMidiId().has_value());
}

TEST_CASE("MidiLibraryStore does not overwrite malformed metadata during import",
          "[app][midi-library]")
{
  const auto root = uniqueLibraryRoot();
  const auto libraryRoot = root / "library";
  const auto metadataPath = libraryRoot / "midi-library.json";
  const auto sourcePath =
    writeSourceMidi(root / "source", "new.mid", {'M', 'T', 'h', 'd', 40, 41, 42, 43});
  MidiLibraryStore store(libraryRoot);
  writeText(metadataPath, "{ invalid json");
  const auto originalMetadata = readBytes(metadataPath);

  const auto result = store.importFile(sourcePath);

  CHECK_FALSE(result.has_value());
  CHECK(readBytes(metadataPath) == originalMetadata);
}

TEST_CASE("MidiLibraryStore does not overwrite incomplete metadata during import",
          "[app][midi-library]")
{
  const auto root = uniqueLibraryRoot();
  const auto libraryRoot = root / "library";
  const auto metadataPath = libraryRoot / "midi-library.json";
  const auto sourcePath =
    writeSourceMidi(root / "source", "new.mid", {'M', 'T', 'h', 'd', 44, 45, 46, 47});
  MidiLibraryStore store(libraryRoot);
  writeText(metadataPath, R"({"version":1})");
  const auto originalMetadata = readBytes(metadataPath);

  const auto result = store.importFile(sourcePath);

  CHECK_FALSE(result.has_value());
  CHECK(readBytes(metadataPath) == originalMetadata);
}

TEST_CASE("MidiLibraryStore drops invalid metadata entries while reading",
          "[app][midi-library]")
{
  const auto root = uniqueLibraryRoot();
  const auto libraryRoot = root / "library";
  const auto metadataPath = libraryRoot / "midi-library.json";
  const auto validId = std::string{"4-0123456789abcdef"};
  const auto missingId = std::string{"4-fedcba9876543210"};
  const auto badHashId = std::string{"4-1111111111111111"};
  const auto badTimestampId = std::string{"4-2222222222222222"};
  const auto wrongNameId = std::string{"4-3333333333333333"};
  const auto wrongSizeId = std::string{"4-4444444444444444"};
  writeSourceMidi(libraryRoot / "files", validId + ".mid", {1, 2, 3, 4});
  writeSourceMidi(libraryRoot / "files", badHashId + ".mid", {5, 6, 7, 8});
  writeSourceMidi(libraryRoot / "files", badTimestampId + ".mid", {9, 10, 11, 12});
  writeSourceMidi(libraryRoot / "files", "does-not-match.mid", {13, 14, 15, 16});
  writeSourceMidi(libraryRoot / "files", wrongSizeId + ".mid", {17, 18});
  writeText(metadataPath,
            nlohmann::json{
              {"version", 1},
              {"lastActiveMidiId", missingId},
              {"files",
               nlohmann::json::array(
                 {midiFileMetadata(validId, validId + ".mid", 4),
                  midiFileMetadata(missingId, missingId + ".mid", 4, "fedcba9876543210"),
                  midiFileMetadata(badHashId, badHashId + ".mid", 4, "not-a-hex-hash"),
                  midiFileMetadata(badTimestampId,
                                   badTimestampId + ".mid",
                                   4,
                                   "2222222222222222",
                                   "2026-02-30T12:34:56Z"),
                  midiFileMetadata(wrongNameId, "does-not-match.mid", 4, "3333333333333333"),
                  midiFileMetadata(wrongSizeId, wrongSizeId + ".mid", 4, "4444444444444444")})}}
              .dump(2));
  MidiLibraryStore const store(libraryRoot);
  RecordingDiagnosticSink diagnostics;

  const auto files = store.listImportedFiles(diagnostics);

  REQUIRE(files.size() == 1);
  CHECK(files.front().id == validId);
  CHECK_FALSE(store.lastActiveMidiId(diagnostics).has_value());
  CHECK(hasDiagnosticContaining(diagnostics, "missing copied MIDI file"));
  CHECK(hasDiagnosticContaining(diagnostics, "invalid content hash"));
  CHECK(hasDiagnosticContaining(diagnostics, "invalid timestamp"));
  CHECK(hasDiagnosticContaining(diagnostics, "stored file name does not match id"));
  CHECK(hasDiagnosticContaining(diagnostics, "size does not match"));
  CHECK(hasDiagnosticContaining(diagnostics, "last active MIDI id does not exist"));
}

TEST_CASE("MidiLibraryStore rejects structurally conflicted metadata before writing",
          "[app][midi-library]")
{
  const auto root = uniqueLibraryRoot();
  const auto libraryRoot = root / "library";
  const auto metadataPath = libraryRoot / "midi-library.json";
  const auto duplicateId = std::string{"4-0123456789abcdef"};
  writeSourceMidi(libraryRoot / "files", duplicateId + ".mid", {1, 2, 3, 4});
  writeSourceMidi(libraryRoot / "files", duplicateId + ".midi", {5, 6, 7, 8});
  writeText(metadataPath,
            nlohmann::json{
              {"version", 1},
              {"files",
               nlohmann::json::array({midiFileMetadata(duplicateId, duplicateId + ".mid", 4),
                                       midiFileMetadata(duplicateId,
                                                        duplicateId + ".midi",
                                                        4,
                                                        "fedcba9876543210")})}}
              .dump(2));
  const auto originalMetadata = readBytes(metadataPath);
  const auto sourcePath =
    writeSourceMidi(root / "source", "new.mid", {'M', 'T', 'h', 'd', 60, 61, 62, 63});
  MidiLibraryStore store(libraryRoot);
  RecordingDiagnosticSink diagnostics;

  CHECK(store.listImportedFiles(diagnostics).empty());
  const auto result = store.importFile(sourcePath, diagnostics);

  CHECK_FALSE(result.has_value());
  CHECK(readBytes(metadataPath) == originalMetadata);
  CHECK(hasDiagnosticContaining(diagnostics, "duplicate id"));
}

} // namespace
