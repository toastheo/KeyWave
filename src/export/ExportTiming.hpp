#pragma once

#include <cstdint>

#include "export/ExportSnapshot.hpp"
#include "export/VideoExportSettings.hpp"

class ExportTiming
{
public:
  static constexpr std::uint64_t sampleRate = 48000;

  // Copies all timing inputs. Invalid ranges throw invalid_argument; unrepresentable
  // frame/sample counts throw overflow_error. No GPU, encoder, or live transport needed.
  ExportTiming(const VideoExportSettings& settings, const ExportSnapshot& snapshot);

  [[nodiscard]] std::uint64_t frameCount() const;
  [[nodiscard]] std::uint64_t sampleCount() const;
  [[nodiscard]] double musicalDurationSeconds() const;
  [[nodiscard]] double tailDurationSeconds() const;
  [[nodiscard]] double outputDurationSeconds() const;
  [[nodiscard]] double frameOutputTimeSeconds(std::uint64_t index) const;
  [[nodiscard]] double sampleOutputTimeSeconds(std::uint64_t index) const;
  [[nodiscard]] double frameSourceTimeSeconds(std::uint64_t index) const;
  [[nodiscard]] double sampleSourceTimeSeconds(std::uint64_t index) const;

private:
  // Source time holds at the cut/end, including quantization padding. Audio must
  // release voices ONCE at musicalDurationSeconds() and keep synthesizing using
  // sampleOutputTimeSeconds(); held source times do not retrigger end events.
  [[nodiscard]] double sourceTimeSeconds(double outputTimeSeconds) const;

  int m_fps;
  double m_offset;
  double m_rate;
  double m_start;
  double m_end;
  double m_musicalDuration;
  double m_tailDuration;
  std::uint64_t m_frameCount;
  std::uint64_t m_sampleCount;
};
