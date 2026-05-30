#pragma once

#include "app/AppSettings.hpp"
#include "diagnostics/Diagnostics.hpp"
#include "input/Key.hpp"
#include "playback/PlaybackTransport.hpp"

void applyPlaybackTransportControl(Key key,
                                   PlaybackTransport& transport,
                                   DiagnosticSink& diagnostics,
                                   const PlaybackControlSettings& settings = {});
