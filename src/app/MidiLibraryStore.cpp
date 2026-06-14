#include "app/MidiLibraryStore.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <nlohmann/json.hpp>
#include <optional>
#include <sstream>
#include <utility>

#include "app/SettingsStorage.hpp"

namespace {

// These are the standard published constants for the 64-bit Fowler–Noll–Vo hash family:
// fnvOffsetBasis is the standard starting value for FNV-1a 64-bit.
// fnvPrime is the standard multiplier for FNV-1a 64-bit.
// For more information: https://en.wikipedia.org/wiki/Fowler%E2%80%93Noll%E2%80%93Vo_hash_function
constexpr std::uint64_t fnvOffsetBasis = 14695981039346656037ull;
constexpr std::uint64_t fnvPrime = 1099511628211ull;

struct FileSignature
{
  std::string contentHash;
  std::uintmax_t sizeBytes = 0;
};

struct FileComparisonPaths
{
  std::filesystem::path source;
  std::filesystem::path importedCopy;
};

struct MidiLibraryState
{
  std::vector<ImportedMidiFile> files;
  std::optional<std::string> lastActiveMidiId;
};

std::string lowercaseHex(const std::uint64_t value)
{
  std::ostringstream output;
  output << std::hex << std::nouppercase << std::setfill('0') << std::setw(16) << value;
  return output.str();
}

std::string lowercaseHexSize(const std::uintmax_t value)
{
  std::ostringstream output;
  output << std::hex << std::nouppercase << value;
  return output.str();
}

bool tryGetUtcTime(const std::time_t time, std::tm& utc)
{
#if defined(_WIN32)
  return gmtime_s(&utc, &time) == 0;
#else
  return gmtime_r(&time, &utc) != nullptr;
#endif
}

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

std::optional<FileSignature> computeSignature(const std::filesystem::path& path,
                                              DiagnosticSink& diagnostics)
{
  std::ifstream input(path, std::ios::binary);
  if (!input) {
    reportWarning(diagnostics, "Warning: could not open MIDI file for import: " + path.string());
    return std::nullopt;
  }

  std::uint64_t hash = fnvOffsetBasis;
  std::uintmax_t size = 0;
  std::array<char, 8192> buffer{}; // 8 KiB chunk size
  while (input) {
    input.read(buffer.data(), buffer.size());
    const auto bytesRead = input.gcount();
    for (std::streamsize index = 0; index < bytesRead; ++index) {
      hash ^= static_cast<unsigned char>(buffer[static_cast<std::size_t>(index)]);
      hash *= fnvPrime;
    }
    size += static_cast<std::uintmax_t>(bytesRead);
  }

  if (!input.eof()) {
    reportWarning(diagnostics, "Warning: could not read MIDI file for import: " + path.string());
    return std::nullopt;
  }

  return FileSignature{
    .contentHash = lowercaseHex(hash),
    .sizeBytes = size,
  };
}

bool sameFileBytes(const FileComparisonPaths& paths, DiagnosticSink& diagnostics)
{
  std::ifstream leftInput(paths.source, std::ios::binary);
  std::ifstream rightInput(paths.importedCopy, std::ios::binary);
  if (!leftInput || !rightInput) {
    return false;
  }

  std::array<char, 8192> leftBuffer{};  // 8 KiB
  std::array<char, 8192> rightBuffer{}; // 8 KiB
  while (leftInput && rightInput) {
    leftInput.read(leftBuffer.data(), leftBuffer.size());
    rightInput.read(rightBuffer.data(), rightBuffer.size());
    const auto leftCount = leftInput.gcount();
    const auto rightCount = rightInput.gcount();
    if (leftCount != rightCount) {
      return false;
    }
    if (!std::equal(leftBuffer.begin(),
                    leftBuffer.begin() + leftCount,
                    rightBuffer.begin(),
                    rightBuffer.begin() + rightCount)) {
      return false;
    }
  }

  if (!leftInput.eof() || !rightInput.eof()) {
    reportWarning(diagnostics, "Warning: could not compare imported MIDI file bytes.");
    return false;
  }

  return true;
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

MidiLibraryState loadLibraryState(const std::filesystem::path& metadataPath,
                                  DiagnosticSink& diagnostics)
{
  try {
    if (!std::filesystem::exists(metadataPath)) {
      return {};
    }

    std::ifstream input(metadataPath);
    if (!input) {
      reportWarning(diagnostics,
                    "Warning: could not open MIDI library metadata: " + metadataPath.string());
      return {};
    }

    const auto json = nlohmann::json::parse(input);
    if (!json.is_object()) {
      reportWarning(diagnostics,
                    "Warning: malformed MIDI library metadata, using empty library: " +
                      metadataPath.string());
      return {};
    }

    MidiLibraryState state;
    if (const auto activeIter = json.find("lastActiveMidiId");
        activeIter != json.end() && activeIter->is_string()) {
      state.lastActiveMidiId = activeIter->get<std::string>();
    }

    const auto filesIter = json.find("files");
    if (filesIter == json.end() || !filesIter->is_array()) {
      reportWarning(diagnostics,
                    "Warning malformed MIDI library metadata, using empty library: " +
                      metadataPath.string());
      return {};
    }

    for (const auto& fileJson : *filesIter) {
      if (auto file = importedMidiFileFromJson(fileJson); file.has_value()) {
        state.files.push_back(std::move(*file));
      }
      else {
        reportWarning(diagnostics,
                      "Warning: ignored malformed MIDI library metadata entry: " +
                        metadataPath.string());
      }
    }
    return state;
  }
  catch (const nlohmann::json::exception& exception) {
    std::ostringstream message;
    message << "Warning: malformed MIDI library metadata, using empty library: " << metadataPath
            << " (" << exception.what() << ")";
    reportWarning(diagnostics, message.str());
  }
  catch (const std::exception& exception) {
    std::ostringstream message;
    message << "Warning: could not load MIDI library metadata, using empty library: "
            << metadataPath << " (" << exception.what() << ")";
    reportWarning(diagnostics, message.str());
  }

  return {};
}

bool saveLibraryState(const std::filesystem::path& metadataPath,
                      const MidiLibraryState& state,
                      DiagnosticSink& diagnostics)
{
  try {
    if (!metadataPath.parent_path().empty()) {
      std::filesystem::create_directories(metadataPath.parent_path());
    }

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
                      "Warning: could not write MIDI library metadata " + tempPath.string());
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

bool idExists(const std::vector<ImportedMidiFile>& files, const std::string_view id)
{
  return std::ranges::any_of(files, [id](const ImportedMidiFile& file) { return file.id == id; });
}

std::string uniqueIdFor(const FileSignature& signature, const std::vector<ImportedMidiFile>& files)
{
  auto baseId = lowercaseHexSize(signature.sizeBytes) + "-" + signature.contentHash;
  if (!idExists(files, baseId)) {
    return baseId;
  }

  for (int suffix = 2;; ++suffix) {
    const auto candidate = baseId + "-" + std::to_string(suffix);
    if (!idExists(files, candidate)) {
      return candidate;
    }
  }
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
  const std::filesystem::path& sourcePath, DiagnosticSink& diagnostics) const
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

  auto signature = computeSignature(sourcePath, diagnostics);
  if (!signature.has_value()) {
    return std::nullopt;
  }

  auto state = loadLibraryState(metadataPath(), diagnostics);
  for (const auto& file : state.files) {
    if (file.contentHash == signature->contentHash && file.sizeBytes == signature->sizeBytes &&
        std::filesystem::exists(storedFilePath(file), errorCode) &&
        sameFileBytes(FileComparisonPaths{.source = sourcePath,
                                          .importedCopy = storedFilePath(file)},
                      diagnostics)) {
      return MidiImportResult{
        .file = file,
        .alreadyImported = true,
      };
    }
  }

  const auto id = uniqueIdFor(*signature, state.files);
  const auto storedFileName = id + sourcePath.extension().string();
  const auto timestamp = utcTimestamp();
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

  state.files.push_back(importedFile);
  if (!saveLibraryState(metadataPath(), state, diagnostics)) {
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
  return loadLibraryState(metadataPath(), diagnostics).files;
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
  return loadLibraryState(metadataPath(), diagnostics).lastActiveMidiId;
}

bool MidiLibraryStore::setLastActiveMidiId(std::string_view id, DiagnosticSink& diagnostics) const
{
  auto state = loadLibraryState(metadataPath(), diagnostics);
  const auto iter =
    std::ranges::find_if(state.files, [id](const ImportedMidiFile& file) { return file.id == id; });
  if (iter == state.files.end()) {
    reportWarning(diagnostics,
                  "Warning: could not set last active MIDI file: imported MIDI id not found.");
    return false;
  }

  const auto timestamp = utcTimestamp();
  if (!timestamp.has_value()) {
    reportWarning(diagnostics,
                  "Warning: could not set last active MIDI file: timestamp unavailable.");
    return false;
  }

  iter->lastOpenedAt = *timestamp;
  state.lastActiveMidiId = std::string{id};
  return saveLibraryState(metadataPath(), state, diagnostics);
}

std::filesystem::path MidiLibraryStore::metadataPath() const
{
  return m_rootPath / "midi-library.json";
}

std::filesystem::path MidiLibraryStore::filesPath() const
{
  return m_rootPath / "files";
}

std::filesystem::path MidiLibraryStore::storedFilePath(const ImportedMidiFile& file) const
{
  return filesPath() / file.storedFileName;
}
