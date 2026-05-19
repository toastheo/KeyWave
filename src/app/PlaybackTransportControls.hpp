#pragma once

#include <iosfwd>

#include "app/AppSettings.hpp"
#include "input/Key.hpp"
#include "playback/PlaybackTransport.hpp"

void applyPlaybackTransportControl(Key key,
                                   PlaybackTransport& transport,
                                   std::ostream& output,
                                   const PlaybackControlSettings& settings = {});
