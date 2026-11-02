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
  // Receives the full output size; ImGui downsamples the returned texture for display.
  using PreviewRenderer = std::function<std::uint64_t(FramebufferSize)>;
  static VideoExportAction render(const VideoExportController& controller,
                                  bool hasTimeline,
                                  VideoExportSettings& settings,
                                  const PreviewRenderer& previewRenderer);
};
