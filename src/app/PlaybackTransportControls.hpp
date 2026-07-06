#pragma once

#include "app/AppSettings.hpp"
#include "input/Key.hpp"
#include "midi/MidiTypes.hpp"

class DiagnosticSink;
class PlaybackTransport;
class TimelineAudioScheduler;

void applyPlaybackTransportControl(Key key,
                                   PlaybackTransport& transport,
                                   DiagnosticSink& diagnostics,
                                   TimelineAudioScheduler& audioScheduler,
                                   const PlaybackControlSettings& settings = {},
                                   double sourceBpm = defaultMidiBpm);
