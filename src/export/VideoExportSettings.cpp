#include "export/VideoExportSettings.hpp"

#include <cmath>
#include <stdexcept>

void validateVideoExportSettings(const VideoExportSettings& settings)
{
  if (settings.fps != 30 && settings.fps != 60) {
    throw std::invalid_argument("Export FPS must be 30 or 60.");
  }
  if (settings.quality != ExportQuality::Medium && settings.quality != ExportQuality::High &&
      settings.quality != ExportQuality::Best) {
    throw std::invalid_argument("Unknown export quality.");
  }
  if (settings.resolution != ExportResolution::P720 &&
      settings.resolution != ExportResolution::P1080 &&
      settings.resolution != ExportResolution::P2160) {
    throw std::invalid_argument("Unknown export resolution.");
  }
  if (settings.aspectRatio != ExportAspectRatio::Landscape &&
      settings.aspectRatio != ExportAspectRatio::Portrait &&
      settings.aspectRatio != ExportAspectRatio::Square) {
    throw std::invalid_argument("Unknown export aspect ratio.");
  }
  if (settings.range.mode != ExportRangeMode::FullSong &&
      settings.range.mode != ExportRangeMode::Clip) {
    throw std::invalid_argument("Unknown export range mode.");
  }
  if (!std::isfinite(settings.range.startSeconds) || !std::isfinite(settings.range.endSeconds)) {
    throw std::invalid_argument("Export range must be finite.");
  }
}

FramebufferSize exportFramebufferSize(const VideoExportSettings& settings)
{
  validateVideoExportSettings(settings);
  const auto height = settings.resolution == ExportResolution::P720    ? 720
                      : settings.resolution == ExportResolution::P1080 ? 1080
                                                                       : 2160;
  const auto width = height * 16 / 9;
  if (settings.aspectRatio == ExportAspectRatio::Portrait) {
    return {height, width};
  }
  return {settings.aspectRatio == ExportAspectRatio::Square ? height : width, height};
}
