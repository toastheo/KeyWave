#pragma once

#include <cstdint>

#include "app/VideoExportController.hpp"

enum class VideoExportAction : std::uint8_t { None, Start, Cancel };

class VideoExportControls
{
public:
  static VideoExportAction render(const VideoExportController& controller, bool hasTimeline);
};
