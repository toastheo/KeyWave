#include "app/PlaybackTransportControls.hpp"

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

void applyPlaybackTransportControl(const Key key,
                                   PlaybackTransport& transport,
                                   std::ostream& output)
{
  switch (key) {
    case Key::Space:
      if (transport.state() == PlaybackState::Playing) {
        transport.pause();
        output << "Playback paused at " << transport.currentTimeSeconds() << "s.\n";
      }
      else {
        transport.play();
        output << "Playback started at " << transport.currentTimeSeconds() << "s.\n";
      }
      break;

    case Key::R:
      transport.seek(0.0);
      transport.play();
      output << "Playback restarted.\n";
      break;

    case Key::S:
      transport.stop();
      output << "Playback stopped.\n";
      break;

    case Key::Left:
      transport.seek(transport.currentTimeSeconds() - kSeekStepSeconds);
      output << "Playback seeked to " << transport.currentTimeSeconds() << "s.\n";
      break;

    case Key::Right:
      transport.seek(transport.currentTimeSeconds() + kSeekStepSeconds);
      output << "Playback seeked to " << transport.currentTimeSeconds() << "s.\n";
      break;

    case Key::Up:
      transport.setPlaybackRate(clampedPlaybackRate(transport.playbackRate() + kPlaybackRateStep));
      output << "Playback rate set to " << transport.playbackRate() << "x.\n";
      break;

    case Key::Down:
      transport.setPlaybackRate(clampedPlaybackRate(transport.playbackRate() - kPlaybackRateStep));
      output << "Playback rate set to " << transport.playbackRate() << "x.\n";
      break;
  }
}
