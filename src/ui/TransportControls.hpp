#pragma once

#include "app/AppSettings.hpp"
#include "midi/MidiTypes.hpp"
#include "playback/PlaybackTransport.hpp"

class TransportControls
{
public:
  static void render(PlaybackTransport& transport,
                     double durationSeconds,
                     const PlaybackControlSettings& settings = {},
                     double sourceBpm = defaultMidiBpm);
};
