#pragma once

#include "app/AppSettings.hpp"
#include "diagnostics/Diagnostics.hpp"
#include "playback/PlaybackTransport.hpp"

enum class PlaybackTransportAction : std::uint8_t
{
  TogglePlayPause,
  Restart,
  Stop,
  SeekBackward,
  SeekForward,
  IncreasePlaybackRate,
  DecreasePlaybackRate,
};

void applyPlaybackTransportAction(PlaybackTransportAction action,
                                  PlaybackTransport& transport,
                                  const PlaybackControlSettings& settings = {});
void reportPlaybackTransportAction(PlaybackTransportAction action,
                                   const PlaybackTransport& transport,
                                   DiagnosticSink& diagnostics);
