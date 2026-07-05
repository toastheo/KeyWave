#include "platform/MidiFileDialog.hpp"

#include <array>
#include <filesystem>
#include <memory>
#include <nfd.h>
#include <optional>
#include <sstream>
#include <string>

#include "diagnostics/Diagnostics.hpp"

namespace {

void reportNfdError(const char* context, DiagnosticSink& diagnostics)
{
  const char* error = NFD_GetError();
  std::ostringstream message;
  message << context;
  if (error != nullptr) {
    message << ": " << error;
  }
  reportError(diagnostics, message.str());
  NFD_ClearError();
}

} // namespace

std::optional<std::filesystem::path> MidiFileDialog::open(DiagnosticSink& diagnostics)
{
  if (NFD_Init() != NFD_OKAY) {
    reportNfdError("MIDI file dialog failed to initialize", diagnostics);
    return std::nullopt;
  }

  struct NfdSession
  {
    ~NfdSession()
    {
      NFD_Quit();
    }
  } const session;

  constexpr std::array<nfdfilteritem_t, 1> filters{
    {"MIDI files", "mid,midi,kar"}, // .kar = Karaoke MIDI file (MIDI with lyrics embedded)
  };

  nfdchar_t* selectedPath = nullptr;
  const nfdresult_t result = NFD_OpenDialog(&selectedPath, filters.data(), filters.size(), nullptr);
  std::unique_ptr<nfdchar_t, decltype(&NFD_FreePath)> const selectedPathGuard{selectedPath,
                                                                              NFD_FreePath};
  if (result == NFD_CANCEL) {
    return std::nullopt;
  }

  if (result == NFD_ERROR) {
    reportNfdError("MIDI file dialog failed", diagnostics);
    return std::nullopt;
  }

  const auto* selectedPathBytes = reinterpret_cast<const char8_t*>(selectedPathGuard.get());
  std::filesystem::path path{std::u8string(selectedPathBytes)};
  return path;
}
