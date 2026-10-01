#pragma once

#include <cstdint>

#include "core/CoreTypes.hpp"

enum class ExportQuality : std::uint8_t
{
  Medium,
  High,
  Best
};
enum class ExportResolution : std::uint8_t
{
  P720,
  P1080,
  P2160
};
enum class ExportAspectRatio : std::uint8_t
{
  Landscape,
  Portrait,
  Square
};
enum class ExportRangeMode : std::uint8_t
{
  FullSong,
  Clip
};

struct ExportRange
{
  ExportRangeMode mode = ExportRangeMode::FullSong;
  // Playback seconds, including the visual lead-in. FullSong resolves to [0, duration].
  double startSeconds = 0.0;
  double endSeconds = 0.0;
};

struct VideoExportSettings
{
  int fps = 60;
  ExportQuality quality = ExportQuality::High;
  ExportResolution resolution = ExportResolution::P1080;
  ExportAspectRatio aspectRatio = ExportAspectRatio::Landscape;
  // Capability-aware UI will choose this once offline audio is available.
  bool includeAudio = false;
  ExportRange range;
};

// Invalid options throw std::invalid_argument. Range bounds require ExportTiming.
void validateVideoExportSettings(const VideoExportSettings& settings);
[[nodiscard]] FramebufferSize exportFramebufferSize(const VideoExportSettings& settings);
