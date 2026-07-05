#include "app/MidiLibraryStore.hpp"

#include <algorithm>
#include <cctype>
#include <exception>
#include <filesystem>
#include <optional>
#include <ranges>
#include <sstream>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

#include "app/SettingsStorage.hpp"
#include "app/midi_library/FileOps.hpp"
#include "app/midi_library/Hash.hpp"
#include "app/midi_library/Ids.hpp"
#include "app/midi_library/Metadata.hpp"
#include "diagnostics/Diagnostics.hpp"

namespace {

std::string trimWhitespace(const std::string_view value)
{
  const auto first = std::ranges::find_if(value, [](const unsigned char character) {
    return std::isspace(character) == 0;
  });
  if (first == value.end()) {
    return {};
  }

  const auto last =
    std::ranges::find_if(std::views::reverse(value), [](const unsigned char character) {
      return std::isspace(character) == 0;
    });
  return {first, last.base()};
}

} // namespace

MidiLibraryStore::MidiLibraryStore(std::filesystem::path rootPath)
    : m_rootPath(std::move(rootPath))
{}

std::filesystem::path MidiLibraryStore::defaultRootPath()
{
  return SettingsStorage::defaultSettingsPath().parent_path() / "midi-library";
}

std::optional<MidiImportResult> MidiLibraryStore::importFile(
  const std::filesystem::path& sourcePath, DiagnosticSink& diagnostics)
{
  if (sourcePath.empty()) {
    reportWarning(diagnostics, "Warning: MIDI import failed: file path is empty.");
    return std::nullopt;
  }

  std::error_code errorCode;
  if (!std::filesystem::exists(sourcePath, errorCode)) {
    std::ostringstream message;
    message << "Warning: MIDI import failed: file does not exist: " << sourcePath.string();
    if (errorCode) {
      message << " (" << errorCode.message() << ')';
    }
    reportWarning(diagnostics, message.str());
    return std::nullopt;
  }

  if (!std::filesystem::is_regular_file(sourcePath, errorCode)) {
    std::ostringstream message;
    message << "Warning: MIDI import failed: path is not a regular file: " << sourcePath.string();
    if (errorCode) {
      message << " (" << errorCode.message() << ')';
    }
    reportWarning(diagnostics, message.str());
    return std::nullopt;
  }

  if (!midi_library::hasAllowedMidiExtension(sourcePath)) {
    reportWarning(diagnostics,
                  "Warning: MIDI import failed: unsupported file extension: " +
                    sourcePath.string());
    return std::nullopt;
  }

  auto state = midi_library::loadLibraryStateForWrite(metadataPath(), diagnostics);
  if (!state.has_value()) {
    return std::nullopt;
  }

  auto signature = midi_library::computeSignature(sourcePath, diagnostics);
  if (!signature.has_value()) {
    return std::nullopt;
  }

  // First, we compare the hash and the size. If they're different, it can't possibly be the same
  // file, and we can move on right away. We still perform a byte-by-byte comparison afterward to
  // account for rare hash collisions.
  for (const auto& file : state->files) {
    if (file.contentHash == signature->contentHash && file.sizeBytes == signature->sizeBytes &&
        std::filesystem::exists(storedFilePath(file), errorCode) &&
        midi_library::sameFileBytes(
          midi_library::FileComparisonPaths{
            .source = sourcePath,
            .importedCopy = storedFilePath(file),
          },
          diagnostics)) {
      return MidiImportResult{
        .file = file,
        .alreadyImported = true,
      };
    }
  }

  const auto id = midi_library::uniqueIdFor(*signature, state->files);
  const auto storedFileName = id + midi_library::normalizeExtension(sourcePath);
  const auto timestamp = midi_library::utcTimestamp();
  if (!timestamp.has_value()) {
    reportWarning(diagnostics, "Warning: MIDI import failed: could not create import timestamp.");
    return std::nullopt;
  }

  ImportedMidiFile importedFile{
    .id = id,
    .displayName = sourcePath.stem().string(),
    .storedFileName = storedFileName,
    .originalFileName = sourcePath.filename().string(),
    .contentHash = signature->contentHash,
    .sizeBytes = signature->sizeBytes,
    .importedAt = *timestamp,
    .lastOpenedAt = *timestamp,
  };

  try {
    std::filesystem::create_directories(filesPath());
    std::filesystem::copy_file(sourcePath,
                               storedFilePath(importedFile),
                               std::filesystem::copy_options::none);
  }
  catch (const std::exception& exception) {
    std::ostringstream message;
    message << "Warning: MIDI import failed while copying file: " << sourcePath << " ("
            << exception.what() << ")";
    reportWarning(diagnostics, message.str());
    return std::nullopt;
  }

  state->files.push_back(importedFile);
  if (!midi_library::saveLibraryState(metadataPath(), *state, diagnostics)) {
    std::error_code removeError;
    std::filesystem::remove(storedFilePath(importedFile), removeError);
    return std::nullopt;
  }

  return MidiImportResult{
    .file = std::move(importedFile),
    .alreadyImported = false,
  };
}

std::vector<ImportedMidiFile> MidiLibraryStore::listImportedFiles(DiagnosticSink& diagnostics) const
{
  return midi_library::loadLibraryState(metadataPath(), diagnostics).files;
}

std::optional<std::filesystem::path> MidiLibraryStore::importedFilePath(
  const std::string_view id, DiagnosticSink& diagnostics) const
{
  const auto file = findById(id, diagnostics);
  if (!file.has_value()) {
    return std::nullopt;
  }

  auto path = storedFilePath(*file);
  if (std::error_code errorCode; !std::filesystem::exists(path, errorCode) ||
                                 !std::filesystem::is_regular_file(path, errorCode)) {
    return std::nullopt;
  }

  return path;
}

std::optional<ImportedMidiFile> MidiLibraryStore::findById(std::string_view id,
                                                           DiagnosticSink& diagnostics) const
{
  const auto files = listImportedFiles(diagnostics);
  const auto iter =
    std::ranges::find_if(files, [id](const ImportedMidiFile& file) { return file.id == id; });
  if (iter == files.end()) {
    return std::nullopt;
  }

  return *iter;
}

std::optional<std::string> MidiLibraryStore::lastActiveMidiId(DiagnosticSink& diagnostics) const
{
  return midi_library::loadLibraryState(metadataPath(), diagnostics).lastActiveMidiId;
}

bool MidiLibraryStore::setLastActiveMidiId(std::string_view id, DiagnosticSink& diagnostics)
{
  auto state = midi_library::loadLibraryStateForWrite(metadataPath(), diagnostics);
  if (!state.has_value()) {
    return false;
  }

  const auto iter = std::ranges::find_if(state->files, [id](const ImportedMidiFile& file) {
    return file.id == id;
  });
  if (iter == state->files.end()) {
    reportWarning(diagnostics,
                  "Warning: could not set last active MIDI file: imported MIDI id not found.");
    return false;
  }

  const auto timestamp = midi_library::utcTimestamp();
  if (!timestamp.has_value()) {
    reportWarning(diagnostics,
                  "Warning: could not set last active MIDI file: timestamp unavailable.");
    return false;
  }

  iter->lastOpenedAt = *timestamp;
  state->lastActiveMidiId = std::string{id};
  return midi_library::saveLibraryState(metadataPath(), *state, diagnostics);
}

// Renaming is ui exclusive since we only modify the display name here.
bool MidiLibraryStore::renameImportedMidiFile(std::string_view id,
                                              const std::string_view displayName,
                                              DiagnosticSink& diagnostics)
{
  const auto trimmedDisplayName = trimWhitespace(displayName);
  if (trimmedDisplayName.empty()) {
    reportWarning(diagnostics,
                  "Warning: could not rename imported MIDI file: display name is empty.");
    return false;
  }

  auto state = midi_library::loadLibraryStateForWrite(metadataPath(), diagnostics);
  if (!state.has_value()) {
    return false;
  }

  const auto iter = std::ranges::find_if(state->files, [id](const ImportedMidiFile& file) {
    return file.id == id;
  });
  if (iter == state->files.end()) {
    reportWarning(diagnostics,
                  "Warning: could not rename imported MIDI file: imported MIDI id not found.");
    return false;
  }

  iter->displayName = trimmedDisplayName;
  return midi_library::saveLibraryState(metadataPath(), *state, diagnostics);
}

bool MidiLibraryStore::removeImportedMidiFile(std::string_view id, DiagnosticSink& diagnostics)
{
  auto state = midi_library::loadLibraryStateForWrite(metadataPath(),
                                                      diagnostics,
                                                      midi_library::StoredFileValidation::Skip);
  if (!state.has_value()) {
    return false;
  }

  const auto iter = std::ranges::find_if(state->files, [id](const ImportedMidiFile& file) {
    return file.id == id;
  });
  if (iter == state->files.end()) {
    reportWarning(diagnostics,
                  "Warning: could not remove imported MIDI file: imported MIDI id not found.");
    return false;
  }

  const auto copiedFilePath = storedFilePath(*iter);
  state->files.erase(iter);
  if (state->lastActiveMidiId.has_value() && *state->lastActiveMidiId == id) {
    state->lastActiveMidiId.reset();
  }

  if (!midi_library::saveLibraryState(metadataPath(), *state, diagnostics)) {
    return false;
  }

  std::error_code errorCode;
  std::filesystem::remove(copiedFilePath, errorCode);
  if (errorCode) {
    std::ostringstream message;
    message << "Warning: removed imported MIDI metadata, but could not delete copied MIDI file: "
            << copiedFilePath << " (" << errorCode.message() << ")";
    reportWarning(diagnostics, message.str());
  }

  return true;
}

std::filesystem::path MidiLibraryStore::metadataPath() const
{
  return m_rootPath / "midi-library.json";
}

std::filesystem::path MidiLibraryStore::filesPath() const
{
  return midi_library::filesPathFor(m_rootPath);
}

std::filesystem::path MidiLibraryStore::storedFilePath(const ImportedMidiFile& file) const
{
  return midi_library::storedFilePathFor(m_rootPath, file);
}
