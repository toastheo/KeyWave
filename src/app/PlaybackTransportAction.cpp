#include "app/PlaybackTransportAction.hpp"

#include <algorithm>
#include <sstream>

#include "app/AppSettings.hpp"
#include "audio/TimelineAudioScheduler.hpp"
#include "diagnostics/Diagnostics.hpp"
#include "playback/PlaybackTransport.hpp"

namespace {

double clampedPlaybackBpm(const double bpm, const PlaybackControlSettings& settings)
{
  return std::clamp(bpm, settings.minPlaybackBpm, settings.maxPlaybackBpm);
}

void adjustPlaybackBpm(PlaybackTransport& transport,
                       const PlaybackControlSettings& settings,
                       const double sourceBpm,
                       const double bpmDelta)
{
  const auto targetBpm = clampedPlaybackBpm(transport.effectiveBpm(sourceBpm) + bpmDelta, settings);
  transport.setEffectiveBpm(sourceBpm, targetBpm);
}

} // namespace

void applyPlaybackTransportAction(const PlaybackTransportAction action,
                                  PlaybackTransport& transport,
                                  TimelineAudioScheduler& audioScheduler,
                                  const PlaybackControlSettings& settings,
                                  const double sourceBpm)
{
  const auto sanitizedSettings = sanitizePlaybackControlSettings(settings);

  switch (action) {
    case PlaybackTransportAction::TogglePlayPause:
      if (transport.state() == PlaybackState::Playing) {
        transport.pause();
        audioScheduler.seek(transport.currentTimeSeconds());
      }
      else {
        transport.play();
      }
      break;

    case PlaybackTransportAction::Restart:
      transport.seek(0.0);
      audioScheduler.stop();
      transport.play();
      break;

    case PlaybackTransportAction::Stop:
      transport.stop();
      audioScheduler.stop();
      break;

    case PlaybackTransportAction::SeekBackward:
      transport.seek(transport.currentTimeSeconds() - sanitizedSettings.seekStepSeconds);
      audioScheduler.seek(transport.currentTimeSeconds());
      break;

    case PlaybackTransportAction::SeekForward:
      transport.seek(transport.currentTimeSeconds() + sanitizedSettings.seekStepSeconds);
      break;

    case PlaybackTransportAction::IncreasePlaybackBpm:
      adjustPlaybackBpm(transport, sanitizedSettings, sourceBpm, sanitizedSettings.playbackBpmStep);
      break;

    case PlaybackTransportAction::DecreasePlaybackBpm:
      adjustPlaybackBpm(transport, sanitizedSettings, sourceBpm, -sanitizedSettings.playbackBpmStep);
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
