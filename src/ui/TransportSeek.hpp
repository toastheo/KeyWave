#pragma once

class PlaybackTransport;
class TimelineAudioScheduler;

void previewTransportSeek(PlaybackTransport& transport,
                          double timeSeconds,
                          TimelineAudioScheduler& audioScheduler);

void commitTransportSeek(const PlaybackTransport& transport,
                         TimelineAudioScheduler& audioScheduler);
