#include "app/midi_library/FileOps.hpp"

#include <filesystem>
#include <sstream>
#include <system_error>

#include "app/MidiLibraryStore.hpp"
#include "diagnostics/Diagnostics.hpp"

namespace midi_library {

std::filesystem::path filesPathFor(const std::filesystem::path& libraryRoot)
{
  return libraryRoot / "files";
}

std::filesystem::path storedFilePathFor(const std::filesystem::path& libraryRoot,
                                        const ImportedMidiFile& file)
{
  return filesPathFor(libraryRoot) / file.storedFileName;
}

bool storedFileExistsWithExpectedSize(const std::filesystem::path& path,
                                      const ImportedMidiFile& file,
                                      DiagnosticSink& diagnostics)
{
  std::error_code errorCode;
  if (!std::filesystem::exists(path, errorCode)) {
    std::ostringstream message;
    message << "Warning: MIDI library metadata references missing copied MIDI file: " << path;
    if (errorCode) {
      message << " (" << errorCode.message() << ')';
    }
    reportWarning(diagnostics, message.str());
    return false;
  }

  if (!std::filesystem::is_regular_file(path, errorCode)) {
    std::ostringstream message;
    message << "Warning: MIDI library metadata copied MIDI path is not a regular file: " << path;
    if (errorCode) {
      message << " (" << errorCode.message() << ')';
    }
    reportWarning(diagnostics, message.str());
    return false;
  }

  const auto actualSize = std::filesystem::file_size(path, errorCode);
  if (errorCode) {
    std::ostringstream message;
    message << "Warning: could not validate copied MIDI file size: " << path << " ("
            << errorCode.message() << ')';
    reportWarning(diagnostics, message.str());
    return false;
  }

  if (actualSize != file.sizeBytes) {
    std::ostringstream message;
    message << "Warning: MIDI library metadata size does not match copied MIDI file: " << path;
    reportWarning(diagnostics, message.str());
    return false;
  }

  return true;
}

} // namespace midi_library
