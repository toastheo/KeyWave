#pragma once

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>

#include "diagnostics/Diagnostics.hpp"

namespace midi_library {

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

[[nodiscard]] std::optional<FileSignature> computeSignature(const std::filesystem::path& path,
                                                            DiagnosticSink& diagnostics);
[[nodiscard]] bool sameFileBytes(const FileComparisonPaths& paths, DiagnosticSink& diagnostics);
[[nodiscard]] std::string lowercaseHexSize(std::uintmax_t value);

} // namespace midi_library
