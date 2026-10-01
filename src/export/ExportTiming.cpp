#include "export/ExportTiming.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

ExportTiming::ExportTiming(const VideoExportSettings& settings, const ExportSnapshot& snapshot)
    : m_fps(settings.fps)
    , m_offset(snapshot.timelineOffsetSeconds)
    , m_rate(snapshot.playbackRate)
    , m_start(settings.range.mode == ExportRangeMode::FullSong ? 0.0 : settings.range.startSeconds)
    , m_end(settings.range.mode == ExportRangeMode::FullSong ? snapshot.playbackDurationSeconds()
                                                             : settings.range.endSeconds)
    , m_musicalDuration((m_end - m_start) / m_rate)
    , m_tailDuration(settings.range.mode == ExportRangeMode::FullSong ? 2.0 : 0.0)
{
  validateVideoExportSettings(settings);
  if (m_start < 0.0 || m_start >= m_end || m_end > snapshot.playbackDurationSeconds()) {
    throw std::invalid_argument("Export range must satisfy 0 <= start < end <= song duration.");
  }
  // Compute in the same double precision as the public seconds contract: widening
  // a rounded 0.1 before multiplying by 30 would spuriously add an extra frame.
  const auto frames = std::ceil((m_musicalDuration + m_tailDuration) * m_fps);
  // 2^64 is exact, unlike UINT64_MAX when converted to double.
  if (!std::isfinite(frames) || frames >= std::ldexp(1.0, 64)) {
    throw std::overflow_error("Export frame count exceeds its 64-bit counter.");
  }
  m_frameCount = static_cast<std::uint64_t>(std::max(1.0, frames));
  const auto samplesPerFrame = sampleRate / static_cast<std::uint64_t>(m_fps);
  if (m_frameCount > std::numeric_limits<std::uint64_t>::max() / samplesPerFrame) {
    throw std::overflow_error("Export sample count exceeds its 64-bit counter.");
  }
  m_sampleCount = m_frameCount * samplesPerFrame;
}

std::uint64_t ExportTiming::frameCount() const
{
  return m_frameCount;
}
std::uint64_t ExportTiming::sampleCount() const
{
  return m_sampleCount;
}
double ExportTiming::musicalDurationSeconds() const
{
  return m_musicalDuration;
}
double ExportTiming::tailDurationSeconds() const
{
  return m_tailDuration;
}
double ExportTiming::outputDurationSeconds() const
{
  return static_cast<double>(m_frameCount) / m_fps;
}

double ExportTiming::frameOutputTimeSeconds(std::uint64_t index) const
{
  if (index >= m_frameCount) {
    throw std::out_of_range("Export frame index is outside the output.");
  }
  return static_cast<double>(index) / m_fps;
}

double ExportTiming::sampleOutputTimeSeconds(std::uint64_t index) const
{
  if (index >= m_sampleCount) {
    throw std::out_of_range("Export sample index is outside the output.");
  }
  return static_cast<double>(index) / sampleRate;
}

double ExportTiming::sourceTimeSeconds(double outputTimeSeconds) const
{
  return m_offset + std::min(m_end, m_start + m_rate * outputTimeSeconds);
}

double ExportTiming::frameSourceTimeSeconds(std::uint64_t index) const
{
  return sourceTimeSeconds(frameOutputTimeSeconds(index));
}

double ExportTiming::sampleSourceTimeSeconds(std::uint64_t index) const
{
  return sourceTimeSeconds(sampleOutputTimeSeconds(index));
}
