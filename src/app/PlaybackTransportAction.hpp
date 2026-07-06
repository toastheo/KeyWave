#pragma once

#include "app/AppSettings.hpp"
#include "midi/MidiTypes.hpp"

class DiagnosticSink;
class PlaybackTransport;
class TimelineAudioScheduler;

enum class PlaybackTransportAction : std::uint8_t
{
  TogglePlayPause,
  Restart,
  Stop,
  SeekBackward,
  SeekForward,
  IncreasePlaybackBpm,
  DecreasePlaybackBpm,
};

void applyPlaybackTransportAction(PlaybackTransportAction action,
                                  PlaybackTransport& transport,
                                  TimelineAudioScheduler& audioScheduler,
                                  const PlaybackControlSettings& settings = {},
                                  double sourceBpm = defaultMidiBpm);
void reportPlaybackTransportAction(PlaybackTransportAction action,
                                   const PlaybackTransport& transport,
                                   DiagnosticSink& diagnostics,
                                   double sourceBpm = defaultMidiBpm);
