#pragma once

#include <iosfwd>

#include "playback/PlaybackTransport.hpp"

enum class PlaybackTransportAction : std::uint8_t
{
  TogglePlayPause,
  Restart,
  Stop,
  SeekBackwardFiveSeconds,
  SeekForwardFiveSeconds,
  IncreasePlaybackRate,
  DecreasePlaybackRate,
};

void applyPlaybackTransportAction(PlaybackTransportAction action, PlaybackTransport& transport);
void writePlaybackTransportActionLog(PlaybackTransportAction action,
                                     const PlaybackTransport& transport,
                                     std::ostream& output);
