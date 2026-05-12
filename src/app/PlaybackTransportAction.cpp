#include "app/PlaybackTransportAction.hpp"

#include <algorithm>
#include <ostream>

namespace {
constexpr double kSeekStepSeconds = 5.0;
constexpr double kPlaybackRateStep = 0.25;
constexpr double kMinimumPlaybackRate = 0.25;
constexpr double kMaximumPlaybackRate = 4.0;

double clampedPlaybackRate(const double rate)
{
  return std::clamp(rate, kMinimumPlaybackRate, kMaximumPlaybackRate);
}
} // namespace

void applyPlaybackTransportAction(const PlaybackTransportAction action,
                                  PlaybackTransport& transport)
{
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

    case PlaybackTransportAction::SeekBackwardFiveSeconds:
      transport.seek(transport.currentTimeSeconds() - kSeekStepSeconds);
      break;

    case PlaybackTransportAction::SeekForwardFiveSeconds:
      transport.seek(transport.currentTimeSeconds() + kSeekStepSeconds);
      break;

    case PlaybackTransportAction::IncreasePlaybackRate:
      transport.setPlaybackRate(clampedPlaybackRate(transport.playbackRate() + kPlaybackRateStep));
      break;

    case PlaybackTransportAction::DecreasePlaybackRate:
      transport.setPlaybackRate(clampedPlaybackRate(transport.playbackRate() - kPlaybackRateStep));
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

    case PlaybackTransportAction::SeekBackwardFiveSeconds:
    case PlaybackTransportAction::SeekForwardFiveSeconds:
      output << "Playback seeked to " << transport.currentTimeSeconds() << "s.\n";
      break;

    case PlaybackTransportAction::IncreasePlaybackRate:
    case PlaybackTransportAction::DecreasePlaybackRate:
      output << "Playback rate set to " << transport.playbackRate() << "x.\n";
      break;
  }
}
