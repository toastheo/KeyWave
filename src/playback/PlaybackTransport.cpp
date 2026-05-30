#include "playback/PlaybackTransport.hpp"

#include <algorithm>
#include <cmath>

PlaybackTransport::PlaybackTransport(DiagnosticSink& diagnostics)
    : m_diagnostics(&diagnostics)
{}

void PlaybackTransport::play()
{
  m_state = PlaybackState::Playing;
}

void PlaybackTransport::pause()
{
  m_state = PlaybackState::Paused;
}

void PlaybackTransport::stop()
{
  m_state = PlaybackState::Stopped;
  m_currentTimeSeconds = 0.0;
}

void PlaybackTransport::seek(const double seconds)
{
  if (!std::isfinite(seconds)) {
    reportWarning(*m_diagnostics, "Playback seek ignored: time must be finite.");
    return;
  }

  m_currentTimeSeconds = std::max(0.0, seconds);
}

void PlaybackTransport::update(const double deltaSeconds)
{
  if (m_state != PlaybackState::Playing || !std::isfinite(deltaSeconds) || deltaSeconds < 0.0) {
    return;
  }

  m_currentTimeSeconds += deltaSeconds * m_playbackRate;
}

double PlaybackTransport::currentTimeSeconds() const
{
  return m_currentTimeSeconds;
}

PlaybackState PlaybackTransport::state() const
{
  return m_state;
}

void PlaybackTransport::setPlaybackRate(const double rate)
{
  if (!std::isfinite(rate) || rate <= 0.0) {
    reportWarning(*m_diagnostics, "Playback rate ignored: rate must be a finite positive value.");
    return;
  }

  m_playbackRate = rate;
}

double PlaybackTransport::playbackRate() const
{
  return m_playbackRate;
}
