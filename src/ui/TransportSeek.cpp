#include "ui/TransportSeek.hpp"

#include "audio/TimelineAudioScheduler.hpp"
#include "playback/PlaybackTransport.hpp"

void previewTransportSeek(PlaybackTransport& transport,
                          const double timeSeconds,
                          TimelineAudioScheduler& audioScheduler)
{
  transport.seek(timeSeconds);
  audioScheduler.seek(transport.currentTimeSeconds(),
                      TimelineAudioScheduler::SeekMode::PositionOnly);
}

void commitTransportSeek(const PlaybackTransport& transport, TimelineAudioScheduler& audioScheduler)
{
  audioScheduler.seek(transport.currentTimeSeconds());
}
