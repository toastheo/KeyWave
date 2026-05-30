#include "app/PlaybackTransportControls.hpp"

#include "app/PlaybackTransportAction.hpp"

#include <optional>

namespace {
std::optional<PlaybackTransportAction> actionFromKey(const Key key)
{
  switch (key) {
    case Key::Space:
      return PlaybackTransportAction::TogglePlayPause;

    case Key::R:
      return PlaybackTransportAction::Restart;

    case Key::S:
      return PlaybackTransportAction::Stop;

    case Key::Left:
      return PlaybackTransportAction::SeekBackward;

    case Key::Right:
      return PlaybackTransportAction::SeekForward;

    case Key::Up:
      return PlaybackTransportAction::IncreasePlaybackRate;

    case Key::Down:
      return PlaybackTransportAction::DecreasePlaybackRate;
  }

  return std::nullopt;
}
} // namespace

void applyPlaybackTransportControl(const Key key,
                                   PlaybackTransport& transport,
                                   DiagnosticSink& diagnostics,
                                   const PlaybackControlSettings& settings)
{
  const auto action = actionFromKey(key);
  if (!action.has_value()) {
    return;
  }

  applyPlaybackTransportAction(*action, transport, settings);
  reportPlaybackTransportAction(*action, transport, diagnostics);
}
