#pragma once

#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

#include "app/MidiLibraryStore.hpp"
#include "app/midi_library/Hash.hpp"

namespace midi_library {

[[nodiscard]] std::string normalizeExtension(const std::filesystem::path& path);
[[nodiscard]] bool hasAllowedMidiExtension(const std::filesystem::path& path);
[[nodiscard]] bool isSafeStoredFileName(const std::string& name);
[[nodiscard]] bool storedFileNameMatchesId(const ImportedMidiFile& file);
[[nodiscard]] bool idExists(const std::vector<ImportedMidiFile>& files, std::string_view id);
[[nodiscard]] std::string uniqueIdFor(const FileSignature& signature,
                                      const std::vector<ImportedMidiFile>& files);

} // namespace midi_library
