#pragma once

#include "app/AppSettings.hpp"
#include "diagnostics/Diagnostics.hpp"
#include "midi/MidiTypes.hpp"
#include "playback/PlaybackTransport.hpp"

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
                                  const PlaybackControlSettings& settings = {},
                                  double sourceBpm = defaultMidiBpm);
void reportPlaybackTransportAction(PlaybackTransportAction action,
                                   const PlaybackTransport& transport,
                                   DiagnosticSink& diagnostics,
                                   double sourceBpm = defaultMidiBpm);
