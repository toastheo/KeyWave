#pragma once

#include "app/AppSettings.hpp"
#include "midi/MidiTypes.hpp"

class PlaybackTransport;
class TimelineAudioScheduler;

class TransportControls
{
public:
  static void render(PlaybackTransport& transport,
                     double durationSeconds,
                     TimelineAudioScheduler& audioScheduler,
                     const PlaybackControlSettings& settings = {},
                     double sourceBpm = defaultMidiBpm);
};
