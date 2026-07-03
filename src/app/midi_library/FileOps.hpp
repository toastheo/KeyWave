#pragma once

#include <filesystem>

#include "app/MidiLibraryStore.hpp"
#include "diagnostics/Diagnostics.hpp"

namespace midi_library {

[[nodiscard]] std::filesystem::path filesPathFor(const std::filesystem::path& libraryRoot);
[[nodiscard]] std::filesystem::path storedFilePathFor(const std::filesystem::path& libraryRoot,
                                                      const ImportedMidiFile& file);
[[nodiscard]] bool storedFileExistsWithExpectedSize(const std::filesystem::path& path,
                                                    const ImportedMidiFile& file,
                                                    DiagnosticSink& diagnostics);

} // namespace midi_library
