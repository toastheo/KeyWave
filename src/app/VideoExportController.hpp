#pragma once

#include <functional>
#include <memory>

#include "export/ExportSnapshot.hpp"
#include "export/ExportTiming.hpp"
#include "export/VideoEncoder.hpp"

// tick() runs on the owning GL/UI thread and renders at most one frame. The
// encoder owns worker/process resources; this controller owns the frozen input.
class VideoExportController final
{
public:
  using FrameProducer = std::function<std::vector<std::uint8_t>(
    const ExportSnapshot&, double sourceSeconds, FramebufferSize)>;

  VideoExportController(std::unique_ptr<VideoEncoder> encoder, FrameProducer producer);
  ~VideoExportController();
  bool start(ExportSnapshot snapshot, const std::filesystem::path& destination);
  void tick();
  void cancel();
  [[nodiscard]] bool active() const;
  [[nodiscard]] VideoEncoderStatus status() const;
  [[nodiscard]] std::uint64_t framesGenerated() const;
  [[nodiscard]] std::uint64_t frameCount() const;
  [[nodiscard]] double outputDurationSeconds() const;
  [[nodiscard]] const std::filesystem::path& destination() const;

private:
  std::unique_ptr<VideoEncoder> m_encoder;
  FrameProducer m_producer;
  std::unique_ptr<ExportSnapshot> m_snapshot;
  std::unique_ptr<ExportTiming> m_timing;
  std::filesystem::path m_destination;
  std::uint64_t m_generated = 0;
  std::uint64_t m_frameCount = 0;
  double m_duration = 0;
  bool m_cancelled = false;
  std::string m_error;
};
