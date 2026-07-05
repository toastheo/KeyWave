#pragma once

#include <cstddef>
#include <vector>

#include "audio/PianoSynth.hpp"
#include "midi/MidiTimeline.hpp"

class TimelineAudioScheduler
{
public:
  explicit TimelineAudioScheduler(PianoSynth& synth);

  void setTimeline(const MidiTimeline& timeline);
  void update(double previousTimeSeconds, double currentTimeSeconds);
  void seek(double timeSeconds);
  void stop();

private:
  enum class CursorBoundary
  {
    IncludeEventsAtTime,
    SkipEventsAtTime,
  };

  enum class EventType
  {
    NoteOff,
    NoteOn,
  };

  struct Event
  {
    double timeSeconds = 0.0;
    EventType type = EventType::NoteOn;
    int pitch = 0;
    int velocity = 0;
  };

  void resetCursor(double timeSeconds, CursorBoundary boundary);

  PianoSynth& m_synth;
  std::vector<Event> m_events;
  std::size_t m_nextEventIndex = 0;
};
