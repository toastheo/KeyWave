#include "platform/VideoSaveDialog.hpp"

#include <filesystem>
#include <memory>
#include <nfd.h>
#include <optional>
#include <string>
#include <system_error>
#include <utility>

#include "diagnostics/Diagnostics.hpp"

VideoSaveSelection VideoSaveDialog::selectionForConfirmedPath(const std::filesystem::path& path)
{
  auto output = path;
  if (!output.has_extension()) {
    output += ".mp4";
  }
  // Native dialogs confirm an existing selected path. Do not extend that consent
  // to a different path produced by an application-side extension fallback.
  std::error_code error;
  const bool allowOverwrite = output == path && std::filesystem::exists(path, error) && !error;
  return {.path = std::move(output), .allowOverwrite = allowOverwrite};
}

std::optional<VideoSaveSelection> VideoSaveDialog::save(DiagnosticSink& diagnostics)
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
  return selectionForConfirmedPath(
    std::filesystem::path(std::u8string(reinterpret_cast<const char8_t*>(guard.get()))));
}
