#include "app/midi_library/Ids.hpp"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

#include "app/MidiLibraryStore.hpp"
#include "app/midi_library/Hash.hpp"

namespace midi_library {

std::string normalizeExtension(const std::filesystem::path& path)
{
  auto extension = path.extension().string();
  std::ranges::transform(extension, extension.begin(), [](unsigned char c) {
    return static_cast<char>(std::tolower(c));
  });
  return extension;
}

bool hasAllowedMidiExtension(const std::filesystem::path& path)
{
  const auto extension = normalizeExtension(path);
  return extension == ".mid" || extension == ".midi" || extension == ".kar";
}

bool isSafeStoredFileName(const std::string& name)
{
  const std::filesystem::path path{name};

  if (name.empty() || name == "." || name == "..") {
    return false;
  }

  if (path.filename() != path || path.is_absolute()) {
    return false;
  }

  if (name.find('/') != std::string::npos || name.find('\\') != std::string::npos) {
    return false;
  }

  return hasAllowedMidiExtension(path);
}

bool storedFileNameMatchesId(const ImportedMidiFile& file)
{
  return std::filesystem::path{file.storedFileName}.stem().string() == file.id;
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

} // namespace midi_library
