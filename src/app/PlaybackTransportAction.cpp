#include "app/PlaybackTransportAction.hpp"

#include <algorithm>
#include <sstream>
#include <string>

namespace {

double clampedPlaybackRate(const double rate, const PlaybackControlSettings& settings)
{
  return std::clamp(rate, settings.minPlaybackRate, settings.maxPlaybackRate);
}
} // namespace

void applyPlaybackTransportAction(const PlaybackTransportAction action,
                                  PlaybackTransport& transport,
                                  const PlaybackControlSettings& settings)
{
  const auto sanitizedSettings = sanitizePlaybackControlSettings(settings);

  switch (action) {
    case PlaybackTransportAction::TogglePlayPause:
      if (transport.state() == PlaybackState::Playing) {
        transport.pause();
      }
      else {
        transport.play();
      }
      break;

    case PlaybackTransportAction::Restart:
      transport.seek(0.0);
      transport.play();
      break;

    case PlaybackTransportAction::Stop:
      transport.stop();
      break;

    case PlaybackTransportAction::SeekBackward:
      transport.seek(transport.currentTimeSeconds() - sanitizedSettings.seekStepSeconds);
      break;

    case PlaybackTransportAction::SeekForward:
      transport.seek(transport.currentTimeSeconds() + sanitizedSettings.seekStepSeconds);
      break;

    case PlaybackTransportAction::IncreasePlaybackRate:
      transport.setPlaybackRate(clampedPlaybackRate(
        transport.playbackRate() + sanitizedSettings.playbackRateStep, sanitizedSettings));
      break;

    case PlaybackTransportAction::DecreasePlaybackRate:
      transport.setPlaybackRate(clampedPlaybackRate(
        transport.playbackRate() - sanitizedSettings.playbackRateStep, sanitizedSettings));
      break;
  }
}

void reportPlaybackTransportAction(const PlaybackTransportAction action,
                                   const PlaybackTransport& transport,
                                   DiagnosticSink& diagnostics)
{
  std::ostringstream message;

  switch (action) {
    case PlaybackTransportAction::TogglePlayPause:
      if (transport.state() == PlaybackState::Playing) {
        message << "Playback started at " << transport.currentTimeSeconds() << "s.";
      }
      else {
        message << "Playback paused at " << transport.currentTimeSeconds() << "s.";
      }
      break;

    case PlaybackTransportAction::Restart:
      message << "Playback restarted.";
      break;

    case PlaybackTransportAction::Stop:
      message << "Playback stopped.";
      break;

    case PlaybackTransportAction::SeekBackward:
    case PlaybackTransportAction::SeekForward:
      message << "Playback seeked to " << transport.currentTimeSeconds() << "s.";
      break;

    case PlaybackTransportAction::IncreasePlaybackRate:
    case PlaybackTransportAction::DecreasePlaybackRate:
      message << "Playback rate set to " << transport.playbackRate() << "x.";
      break;
  }

  reportInfo(diagnostics, message.str());
}
