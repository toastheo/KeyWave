#include "platform/VideoSaveDialog.hpp"

#include <filesystem>
#include <memory>
#include <nfd.h>
#include <optional>
#include <string>

#include "diagnostics/Diagnostics.hpp"

std::optional<std::filesystem::path> VideoSaveDialog::save(DiagnosticSink& diagnostics)
{
  if (NFD_Init() != NFD_OKAY) {
    reportError(diagnostics, "Video save dialog could not initialize");
    NFD_ClearError();
    return std::nullopt;
  }
  struct Session
  {
    ~Session()
    {
      NFD_Quit();
    }
  } const session;
  const nfdu8filteritem_t filter{"MP4 video", "mp4"};
  nfdu8char_t* selected = nullptr;
  const auto result = NFD_SaveDialogU8(&selected, &filter, 1, nullptr, "KeyWave.mp4");
  std::unique_ptr<nfdu8char_t, decltype(&NFD_FreePathU8)> const guard(selected, NFD_FreePathU8);
  if (result == NFD_CANCEL) {
    return std::nullopt;
  }
  if (result != NFD_OKAY) {
    const auto* error = NFD_GetError();
    reportError(diagnostics,
                std::string("Video save dialog failed: ") + (error ? error : "unknown error"));
    NFD_ClearError();
    return std::nullopt;
  }
  auto path = std::filesystem::path(std::u8string(reinterpret_cast<const char8_t*>(guard.get())));
  if (!path.has_extension()) {
    path += ".mp4";
  }
  return path;
}
