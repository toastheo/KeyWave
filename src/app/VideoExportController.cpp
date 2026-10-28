#include "app/VideoExportController.hpp"

#include <stdexcept>
#include <utility>

namespace {
bool isActive(VideoEncoderState state)
{
  return state == VideoEncoderState::Starting || state == VideoEncoderState::Running ||
         state == VideoEncoderState::Finalizing;
}
VideoExportSettings mvpSettings()
{
  VideoExportSettings settings;
  settings.fps = 30;
  return settings;
}
} // namespace

VideoExportController::VideoExportController(std::unique_ptr<VideoEncoder> encoder, FrameProducer producer)
  : m_encoder(std::move(encoder)), m_producer(std::move(producer))
{
  if (!m_encoder || !m_producer) { throw std::invalid_argument("Export needs an encoder and renderer"); }
}
VideoExportController::~VideoExportController() { cancel(); }
bool VideoExportController::start(ExportSnapshot snapshot, const std::filesystem::path& destination)
{
  if (active()) { throw std::logic_error("A video export is already running"); }
  m_error.clear();
  m_cancelled = false;
  m_generated = 0;
  m_frameCount = 0;
  m_duration = 0;
  m_snapshot.reset();
  m_timing.reset();
  m_destination = destination;
  try {
    m_snapshot = std::make_unique<ExportSnapshot>(std::move(snapshot));
    const auto settings = mvpSettings();
    m_timing = std::make_unique<ExportTiming>(settings, *m_snapshot);
    m_frameCount = m_timing->frameCount();
    m_duration = m_timing->outputDurationSeconds();
    m_encoder->start({exportFramebufferSize(settings), settings.fps, m_frameCount}, destination);
    return true;
  }
  catch (const std::exception& error) {
    m_error = error.what();
    m_snapshot.reset();
    m_timing.reset();
    return false;
  }
}
void VideoExportController::tick()
{
  const auto current = m_encoder->status();
  if (!isActive(current.state)) {
    m_snapshot.reset();
    m_timing.reset();
    return;
  }
  if (!m_error.empty() || m_cancelled || !m_timing ||
      current.state != VideoEncoderState::Running || !m_encoder->canAcceptFrame()) { return; }
  try {
    auto pixels = m_producer(*m_snapshot, m_timing->frameSourceTimeSeconds(m_generated),
                             exportFramebufferSize(mvpSettings()));
    if (m_encoder->trySubmit(std::move(pixels))) {
      ++m_generated;
      if (m_generated == m_frameCount) { m_encoder->finish(); }
    }
  }
  catch (const std::exception& error) {
    m_error = error.what();
    m_encoder->cancel();
  }
}
void VideoExportController::cancel()
{
  if (active()) { m_cancelled = true; m_encoder->cancel(); }
}
bool VideoExportController::active() const { return isActive(m_encoder->status().state); }
VideoEncoderStatus VideoExportController::status() const
{
  auto current = m_encoder->status();
  if (!m_error.empty()) { current.state = VideoEncoderState::Failed; current.error = m_error; }
  return current;
}
std::uint64_t VideoExportController::framesGenerated() const { return m_generated; }
std::uint64_t VideoExportController::frameCount() const { return m_frameCount; }
double VideoExportController::outputDurationSeconds() const { return m_duration; }
const std::filesystem::path& VideoExportController::destination() const { return m_destination; }
