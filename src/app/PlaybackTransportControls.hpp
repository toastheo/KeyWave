#pragma once

#include <iosfwd>

#include "input/Key.hpp"
#include "playback/PlaybackTransport.hpp"

void applyPlaybackTransportControl(Key key, PlaybackTransport& transport, std::ostream& output);
