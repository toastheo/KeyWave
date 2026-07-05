#include "app/midi_library/Metadata.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <chrono>
#include <cstdint>
#include <ctime>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <ios>
#include <nlohmann/json.hpp>
#include <optional>
#include <sstream>
#include <string_view>
#include <system_error>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "app/MidiLibraryStore.hpp"
#include "app/midi_library/FileOps.hpp"
#include "app/midi_library/Ids.hpp"
#include "diagnostics/Diagnostics.hpp"

namespace midi_library {
namespace {

bool tryGetUtcTime(const std::time_t time, std::tm& utc)
{
#if defined(_WIN32)
  return gmtime_s(&utc, &time) == 0;
#else
  return gmtime_r(&time, &utc) != nullptr;
#endif
}

bool isHex16(const std::string_view value)
{
  return value.size() == 16 && std::ranges::all_of(value, [](const unsigned char character) {
           return std::isxdigit(character) != 0;
         });
}

std::optional<int> parseFixedWidthNumber(const std::string_view value)
{
  if (value.empty() || !std::ranges::all_of(value, [](const unsigned char character) {
        return std::isdigit(character) != 0;
      })) {
    return std::nullopt;
  }

  int parsed = 0;
  for (const auto character : value) {
    parsed = parsed * 10 + (character - '0');
  }
  return parsed;
}

bool isLeapYear(const int year)
{
  return year % 4 == 0 && (year % 100 != 0 || year % 400 == 0);
}

int daysInMonth(const int year, const int month)
{
  constexpr std::array<int, 12> daysByMonth{31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
  if (month == 2 && isLeapYear(year)) {
    return 29;
  }
  return daysByMonth[static_cast<std::size_t>(month - 1)];
}

bool isUtcTimestamp(const std::string_view value)
{
  if (value.size() != 20 || value[4] != '-' || value[7] != '-' || value[10] != 'T' ||
      value[13] != ':' || value[16] != ':' || value[19] != 'Z') {
    return false;
  }

  const auto year = parseFixedWidthNumber(value.substr(0, 4));
  const auto month = parseFixedWidthNumber(value.substr(5, 2));
  const auto day = parseFixedWidthNumber(value.substr(8, 2));
  const auto hour = parseFixedWidthNumber(value.substr(11, 2));
  const auto minute = parseFixedWidthNumber(value.substr(14, 2));
  const auto second = parseFixedWidthNumber(value.substr(17, 2));
  if (!year.has_value() || !month.has_value() || !day.has_value() || !hour.has_value() ||
      !minute.has_value() || !second.has_value()) {
    return false;
  }

  if (*month < 1 || *month > 12 || *day < 1 || *day > daysInMonth(*year, *month)) {
    return false;
  }

  return *hour >= 0 && *hour <= 23 && *minute >= 0 && *minute <= 59 && *second >= 0 &&
         *second <= 59;
}

bool validateImportedMidiFileMetadata(const ImportedMidiFile& file,
                                      const std::filesystem::path& libraryRoot,
                                      DiagnosticSink& diagnostics,
                                      const StoredFileValidation storedFileValidation)
{
  if (file.id.empty()) {
    reportWarning(diagnostics, "Warning: MIDI library metadata contains empty id.");
    return false;
  }

  if (!isSafeStoredFileName(file.storedFileName)) {
    reportWarning(diagnostics, "Warning: MIDI library metadata contains unsafe stored file name.");
    return false;
  }

  if (!storedFileNameMatchesId(file)) {
    reportWarning(diagnostics,
                  "Warning: MIDI library metadata stored file name does not match id: " + file.id);
    return false;
  }

  if (!isHex16(file.contentHash)) {
    reportWarning(diagnostics,
                  "Warning: MIDI library metadata contains invalid content hash: " + file.id);
    return false;
  }

  if (!isUtcTimestamp(file.importedAt) || !isUtcTimestamp(file.lastOpenedAt)) {
    reportWarning(diagnostics,
                  "Warning: MIDI library metadata contains invalid timestamp: " + file.id);
    return false;
  }

  if (storedFileValidation == StoredFileValidation::Skip) {
    return true;
  }

  return storedFileExistsWithExpectedSize(storedFilePathFor(libraryRoot, file), file, diagnostics);
}

std::optional<MidiLibraryState> validateLibraryState(MidiLibraryState state,
                                                     const std::filesystem::path& libraryRoot,
                                                     DiagnosticSink& diagnostics,
                                                     const MetadataLoadPolicy policy,
                                                     const StoredFileValidation storedFileValidation)
{
  std::vector<ImportedMidiFile> validFiles;
  validFiles.reserve(state.files.size());
  for (auto& file : state.files) {
    if (validateImportedMidiFileMetadata(file, libraryRoot, diagnostics, storedFileValidation)) {
      validFiles.push_back(std::move(file));
    }
    else if (policy == MetadataLoadPolicy::Strict) {
      return std::nullopt;
    }
  }

  std::unordered_map<std::string, int> idCounts;
  std::unordered_map<std::string, int> storedNameCounts;
  for (const auto& file : validFiles) {
    ++idCounts[file.id];
    ++storedNameCounts[file.storedFileName];
  }

  bool hasStructuralConflict = false;
  for (const auto& [id, count] : idCounts) {
    if (count > 1) {
      reportWarning(diagnostics, "Warning: MIDI library metadata contains duplicate id: " + id);
      hasStructuralConflict = true;
    }
  }

  for (const auto& [storedName, count] : storedNameCounts) {
    if (count > 1) {
      reportWarning(diagnostics,
                    "Warning: MIDI library metadata contains duplicate stored file name: " +
                      storedName);
      hasStructuralConflict = true;
    }
  }

  if (hasStructuralConflict && policy == MetadataLoadPolicy::Strict) {
    return std::nullopt;
  }

  if (hasStructuralConflict) {
    std::erase_if(validFiles, [&idCounts, &storedNameCounts](const ImportedMidiFile& file) {
      return idCounts[file.id] > 1 || storedNameCounts[file.storedFileName] > 1;
    });
  }

  std::unordered_set<std::string> ids;
  ids.reserve(validFiles.size());
  for (const auto& file : validFiles) {
    ids.insert(file.id);
  }

  if (state.lastActiveMidiId.has_value() && !ids.contains(*state.lastActiveMidiId)) {
    reportWarning(diagnostics, "Warning: last active MIDI id does not exist in metadata.");
    if (policy == MetadataLoadPolicy::Strict) {
      return std::nullopt;
    }
    state.lastActiveMidiId.reset();
  }

  state.files = std::move(validFiles);
  return state;
}

std::optional<ImportedMidiFile> importedMidiFileFromJson(const nlohmann::json& json)
{
  if (!json.is_object()) {
    return std::nullopt;
  }

  const auto stringValue = [&json](const char* key) -> std::optional<std::string> {
    const auto iter = json.find(key);
    if (iter == json.end() || !iter->is_string()) {
      return std::nullopt;
    }
    return iter->get<std::string>();
  };

  auto id = stringValue("id");
  auto displayName = stringValue("displayName");
  auto storedFileName = stringValue("storedFileName");
  auto originalFileName = stringValue("originalFileName");
  auto contentHash = stringValue("contentHash");
  const auto sizeIter = json.find("sizeBytes");
  auto importedAt = stringValue("importedAt");
  auto lastOpenedAt = stringValue("lastOpenedAt");

  if (!id.has_value() || !displayName.has_value() || !storedFileName.has_value() ||
      !contentHash.has_value() || !originalFileName.has_value() || !importedAt.has_value() ||
      !lastOpenedAt.has_value() || sizeIter == json.end() || !sizeIter->is_number_unsigned()) {
    return std::nullopt;
  }

  if (!isSafeStoredFileName(*storedFileName)) {
    return std::nullopt;
  }

  return ImportedMidiFile{
    .id = std::move(*id),
    .displayName = std::move(*displayName),
    .storedFileName = std::move(*storedFileName),
    .originalFileName = std::move(*originalFileName),
    .contentHash = std::move(*contentHash),
    .sizeBytes = sizeIter->get<std::uintmax_t>(),
    .importedAt = std::move(*importedAt),
    .lastOpenedAt = std::move(*lastOpenedAt),
  };
}

nlohmann::json importedMidiFileToJson(const ImportedMidiFile& file)
{
  return nlohmann::json{
    {"id", file.id},
    {"displayName", file.displayName},
    {"storedFileName", file.storedFileName},
    {"originalFileName", file.originalFileName},
    {"contentHash", file.contentHash},
    {"sizeBytes", file.sizeBytes},
    {"importedAt", file.importedAt},
    {"lastOpenedAt", file.lastOpenedAt},
  };
}

std::optional<MidiLibraryState> tryLoadLibraryState(const std::filesystem::path& metadataPath,
                                                    DiagnosticSink& diagnostics,
                                                    const MetadataLoadPolicy policy,
                                                    const StoredFileValidation storedFileValidation)
{
  try {
    if (!std::filesystem::exists(metadataPath)) {
      return MidiLibraryState{};
    }

    std::ifstream input(metadataPath);
    if (!input) {
      reportWarning(diagnostics,
                    "Warning: could not open MIDI library metadata: " + metadataPath.string());
      return std::nullopt;
    }

    const auto json = nlohmann::json::parse(input);
    if (!json.is_object()) {
      reportWarning(diagnostics,
                    "Warning: malformed MIDI library metadata: " + metadataPath.string());
      return std::nullopt;
    }

    MidiLibraryState state;
    if (const auto activeIter = json.find("lastActiveMidiId");
        activeIter != json.end() && activeIter->is_string()) {
      state.lastActiveMidiId = activeIter->get<std::string>();
    }
    else if (activeIter != json.end() && policy == MetadataLoadPolicy::Strict) {
      reportWarning(diagnostics,
                    "Warning: malformed MIDI library metadata last active id: " +
                      metadataPath.string());
      return std::nullopt;
    }

    const auto filesIter = json.find("files");
    if (filesIter == json.end() || !filesIter->is_array()) {
      reportWarning(diagnostics,
                    "Warning malformed MIDI library metadata: " + metadataPath.string());
      return std::nullopt;
    }

    for (const auto& fileJson : *filesIter) {
      if (auto file = importedMidiFileFromJson(fileJson); file.has_value()) {
        state.files.push_back(std::move(*file));
      }
      else {
        reportWarning(diagnostics,
                      "Warning: ignored malformed MIDI library metadata entry: " +
                        metadataPath.string());
        if (policy == MetadataLoadPolicy::Strict) {
          return std::nullopt;
        }
      }
    }
    return validateLibraryState(
      std::move(state), metadataPath.parent_path(), diagnostics, policy, storedFileValidation);
  }
  catch (const nlohmann::json::exception& exception) {
    std::ostringstream message;
    message << "Warning: malformed MIDI library metadata: " << metadataPath << " ("
            << exception.what() << ")";
    reportWarning(diagnostics, message.str());
  }
  catch (const std::exception& exception) {
    std::ostringstream message;
    message << "Warning: could not load MIDI library metadata: " << metadataPath << " ("
            << exception.what() << ")";
    reportWarning(diagnostics, message.str());
  }

  return std::nullopt;
}

} // namespace

std::optional<std::string> utcTimestamp()
{
  const auto now = std::chrono::system_clock::now();
  const auto time = std::chrono::system_clock::to_time_t(now);

  std::tm utc{};
  if (!tryGetUtcTime(time, utc)) {
    return std::nullopt;
  }

  std::ostringstream output;
  output << std::put_time(&utc, "%Y-%m-%dT%H:%M:%SZ");
  return output.str();
}

MidiLibraryState loadLibraryState(const std::filesystem::path& metadataPath,
                                  DiagnosticSink& diagnostics)
{
  return tryLoadLibraryState(
           metadataPath, diagnostics, MetadataLoadPolicy::Tolerant, StoredFileValidation::Require)
    .value_or(MidiLibraryState{});
}

std::optional<MidiLibraryState> loadLibraryStateForWrite(
  const std::filesystem::path& metadataPath,
  DiagnosticSink& diagnostics,
  const StoredFileValidation storedFileValidation)
{
  return tryLoadLibraryState(
    metadataPath, diagnostics, MetadataLoadPolicy::Strict, storedFileValidation);
}

bool saveLibraryState(const std::filesystem::path& metadataPath,
                      const MidiLibraryState& state,
                      DiagnosticSink& diagnostics)
{
  try {
    if (!metadataPath.parent_path().empty()) {
      std::filesystem::create_directories(metadataPath.parent_path());
    }

    // Write to a temporary file first so a crash does not leave a half-written library index.
    auto tempPath = metadataPath;
    tempPath += ".tmp";

    nlohmann::json filesJson = nlohmann::json::array();
    for (const auto& file : state.files) {
      filesJson.push_back(importedMidiFileToJson(file));
    }

    {
      std::ofstream output(tempPath, std::ios::trunc);
      if (!output) {
        reportWarning(diagnostics,
                      "Warning: could not open temporary MIDI library metadata for writing: " +
                        tempPath.string());
        return false;
      }

      nlohmann::json metadataJson{{"version", 1}, {"files", filesJson}};
      if (state.lastActiveMidiId.has_value()) {
        metadataJson["lastActiveMidiId"] = *state.lastActiveMidiId;
      }

      output << metadataJson.dump(2) << '\n';
      if (!output) {
        reportWarning(diagnostics,
                      "Warning: could not write MIDI library metadata: " + tempPath.string());
        return false;
      }
    }

    std::error_code renameError;
    std::filesystem::rename(tempPath, metadataPath, renameError);
    if (renameError) {
      std::ostringstream message;
      message << "Warning: could not save MIDI library metadata: " << metadataPath << " ("
              << renameError.message() << ")";
      reportWarning(diagnostics, message.str());
      return false;
    }

    return true;
  }
  catch (const std::exception& exception) {
    std::ostringstream message;
    message << "Warning: could not save MIDI library metadata: " << metadataPath << " ("
            << exception.what() << ")";
    reportWarning(diagnostics, message.str());
  }

  return false;
}

} // namespace midi_library
