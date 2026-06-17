#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "diagnostics/Diagnostics.hpp"

struct ImportedMidiFile
{
  std::string id;
  std::string displayName;
  std::string storedFileName;
  std::string originalFileName;
  std::string contentHash;
  std::uintmax_t sizeBytes = 0;
  std::string importedAt;
  std::string lastOpenedAt;
};

struct MidiImportResult
{
  ImportedMidiFile file;
  bool alreadyImported = false;
};

class MidiLibraryStore
{
public:
  explicit MidiLibraryStore(std::filesystem::path rootPath = defaultRootPath());

  [[nodiscard]] static std::filesystem::path defaultRootPath();

  [[nodiscard]] std::optional<MidiImportResult> importFile(
    const std::filesystem::path& sourcePath,
    DiagnosticSink& diagnostics = nullDiagnosticSink()) const;
  [[nodiscard]] std::vector<ImportedMidiFile> listImportedFiles(
    DiagnosticSink& diagnostics = nullDiagnosticSink()) const;
  [[nodiscard]] std::optional<std::filesystem::path> importedFilePath(
    std::string_view id, DiagnosticSink& diagnostics = nullDiagnosticSink()) const;
  [[nodiscard]] std::optional<ImportedMidiFile> findById(
    std::string_view id, DiagnosticSink& diagnostics = nullDiagnosticSink()) const;
  [[nodiscard]] std::optional<std::string> lastActiveMidiId(
    DiagnosticSink& diagnostics = nullDiagnosticSink()) const;
  [[nodiscard]] bool setLastActiveMidiId(std::string_view id,
                                         DiagnosticSink& diagnostics = nullDiagnosticSink()) const;
  [[nodiscard]] bool renameImportedMidiFile(
    std::string_view id,
    std::string_view displayName,
    DiagnosticSink& diagnostics = nullDiagnosticSink()) const;
  [[nodiscard]] bool removeImportedMidiFile(
    std::string_view id, DiagnosticSink& diagnostics = nullDiagnosticSink()) const;

private:
  [[nodiscard]] std::filesystem::path metadataPath() const;
  [[nodiscard]] std::filesystem::path filesPath() const;
  [[nodiscard]] std::filesystem::path storedFilePath(const ImportedMidiFile& file) const;

  std::filesystem::path m_rootPath;
};
