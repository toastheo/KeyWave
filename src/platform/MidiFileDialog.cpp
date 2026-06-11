#include "platform/MidiFileDialog.hpp"

#include <filesystem>
#include <sstream>
#include <string>

#include <nfd.h>

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
    ~NfdSession() { NFD_Quit(); }
  } session;

  constexpr nfdfilteritem_t filters[] = {
    {"MIDI files", "mid,midi,kar"}, // .kar = Karaoke MIDI file (MIDI with lyrics embedded)
  };

  nfdchar_t* selectedPath = nullptr;
  const nfdresult_t result = NFD_OpenDialog(&selectedPath, filters, 1, nullptr);
  if (result == NFD_CANCEL) {
    return std::nullopt;
  }

  if (result == NFD_ERROR) {
    reportNfdError("MIDI file dialog failed", diagnostics);
    return std::nullopt;
  }

  const auto* selectedPathBytes = reinterpret_cast<const char8_t*>(selectedPath);
  std::filesystem::path path{std::u8string(selectedPathBytes)};
  NFD_FreePath(selectedPath);
  return path;
}
