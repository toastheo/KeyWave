#include "app/PlaybackTransportAction.hpp"

#include <algorithm>
#include <sstream>
#include <string>

namespace {

double clampedPlaybackBpm(const double bpm, const PlaybackControlSettings& settings)
{
  return std::clamp(bpm, settings.minPlaybackBpm, settings.maxPlaybackBpm);
}
} // namespace

void applyPlaybackTransportAction(const PlaybackTransportAction action,
                                  PlaybackTransport& transport,
                                  const PlaybackControlSettings& settings,
                                  const double sourceBpm)
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

    case PlaybackTransportAction::IncreasePlaybackBpm:
      transport.setEffectiveBpm(sourceBpm,
                                clampedPlaybackBpm(transport.effectiveBpm(sourceBpm) +
                                                     sanitizedSettings.playbackBpmStep,
                                                   sanitizedSettings));
      break;

    case PlaybackTransportAction::DecreasePlaybackBpm:
      transport.setEffectiveBpm(sourceBpm,
                                clampedPlaybackBpm(transport.effectiveBpm(sourceBpm) -
                                                     sanitizedSettings.playbackBpmStep,
                                                   sanitizedSettings));
      break;
  }
}

void reportPlaybackTransportAction(const PlaybackTransportAction action,
                                   const PlaybackTransport& transport,
                                   DiagnosticSink& diagnostics,
                                   const double sourceBpm)
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

    case PlaybackTransportAction::IncreasePlaybackBpm:
    case PlaybackTransportAction::DecreasePlaybackBpm:
      message << "Playback BPM set to " << transport.effectiveBpm(sourceBpm) << " BPM.";
      break;
  }

  reportInfo(diagnostics, message.str());
}
