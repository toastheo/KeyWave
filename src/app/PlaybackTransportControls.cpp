#include "app/PlaybackTransportControls.hpp"

#include "app/PlaybackTransportAction.hpp"

namespace {
PlaybackTransportAction actionFromKey(const Key key)
{
  switch (key) {
    case Key::Space:
      return PlaybackTransportAction::TogglePlayPause;

    case Key::R:
      return PlaybackTransportAction::Restart;

    case Key::S:
      return PlaybackTransportAction::Stop;

    case Key::Left:
      return PlaybackTransportAction::SeekBackwardFiveSeconds;

    case Key::Right:
      return PlaybackTransportAction::SeekForwardFiveSeconds;

    case Key::Up:
      return PlaybackTransportAction::IncreasePlaybackRate;

    case Key::Down:
      return PlaybackTransportAction::DecreasePlaybackRate;
  }

  return PlaybackTransportAction::Stop;
}
} // namespace

void applyPlaybackTransportControl(const Key key,
                                   PlaybackTransport& transport,
                                   std::ostream& output)
{
  const auto action = actionFromKey(key);
  applyPlaybackTransportAction(action, transport);
  writePlaybackTransportActionLog(action, transport, output);
}
