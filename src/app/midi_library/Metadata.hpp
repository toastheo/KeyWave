#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

#include "app/MidiLibraryStore.hpp"
#include "diagnostics/Diagnostics.hpp"

namespace midi_library {

struct MidiLibraryState
{
  std::vector<ImportedMidiFile> files;
  std::optional<std::string> lastActiveMidiId;
};

enum class MetadataLoadPolicy
{
  Tolerant,
  Strict,
};

enum class StoredFileValidation
{
  Require,
  Skip,
};

[[nodiscard]] std::optional<std::string> utcTimestamp();
[[nodiscard]] MidiLibraryState loadLibraryState(const std::filesystem::path& metadataPath,
                                                DiagnosticSink& diagnostics);
[[nodiscard]] std::optional<MidiLibraryState> loadLibraryStateForWrite(
  const std::filesystem::path& metadataPath,
  DiagnosticSink& diagnostics,
  StoredFileValidation storedFileValidation = StoredFileValidation::Require);
[[nodiscard]] bool saveLibraryState(const std::filesystem::path& metadataPath,
                                    const MidiLibraryState& state,
                                    DiagnosticSink& diagnostics);

} // namespace midi_library
