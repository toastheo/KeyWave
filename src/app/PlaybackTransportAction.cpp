#include "app/PlaybackTransportAction.hpp"

#include <algorithm>
#include <ostream>

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

void writePlaybackTransportActionLog(const PlaybackTransportAction action,
                                     const PlaybackTransport& transport,
                                     std::ostream& output)
{
  switch (action) {
    case PlaybackTransportAction::TogglePlayPause:
      if (transport.state() == PlaybackState::Playing) {
        output << "Playback started at " << transport.currentTimeSeconds() << "s.\n";
      }
      else {
        output << "Playback paused at " << transport.currentTimeSeconds() << "s.\n";
      }
      break;

    case PlaybackTransportAction::Restart:
      output << "Playback restarted.\n";
      break;

    case PlaybackTransportAction::Stop:
      output << "Playback stopped.\n";
      break;

    case PlaybackTransportAction::SeekBackward:
    case PlaybackTransportAction::SeekForward:
      output << "Playback seeked to " << transport.currentTimeSeconds() << "s.\n";
      break;

    case PlaybackTransportAction::IncreasePlaybackRate:
    case PlaybackTransportAction::DecreasePlaybackRate:
      output << "Playback rate set to " << transport.playbackRate() << "x.\n";
      break;
  }
}
