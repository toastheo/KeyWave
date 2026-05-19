#pragma once

#include <iosfwd>

#include "app/AppSettings.hpp"
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
void writePlaybackTransportActionLog(PlaybackTransportAction action,
                                     const PlaybackTransport& transport,
                                     std::ostream& output);
