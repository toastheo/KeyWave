#pragma once

#include <cstdint>
#include <functional>

#include "app/VideoExportController.hpp"
#include "core/CoreTypes.hpp"
#include "export/VideoExportSettings.hpp"

enum class VideoExportAction : std::uint8_t
{
  None,
  Start,
  Cancel
};

class VideoExportControls
{
public:
  // Borrows the latest generated export texture; ImGui downsamples it for display.
  using FrameTextureProvider = std::function<std::uint64_t(FramebufferSize)>;
  static VideoExportAction render(const VideoExportController& controller,
                                  bool hasTimeline,
                                  VideoExportSettings& settings,
                                  const FrameTextureProvider& frameTexture);
};
