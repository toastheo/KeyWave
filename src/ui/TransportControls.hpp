#pragma once

#include "playback/PlaybackTransport.hpp"

class TransportControls
{
public:
  static void render(PlaybackTransport& transport, double durationSeconds);
};
