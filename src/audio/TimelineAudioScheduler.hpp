#pragma once

#include <cstddef>
#include <vector>

#include "audio/PianoSynth.hpp"
#include "midi/MidiTimeline.hpp"

class TimelineAudioScheduler
{
public:
  enum class SeekMode
  {
    RestoreState,
    PositionOnly,
  };

  explicit TimelineAudioScheduler(PianoSynth& synth);

  void setTimeline(const MidiTimeline& timeline, double timelineOffsetSeconds = 0.0);
  void update(double previousTimeSeconds, double currentTimeSeconds);
  void pause();
  void resume();
  void seek(double timeSeconds, SeekMode mode = SeekMode::RestoreState);
  void stop();

private:
  enum class CursorBoundary
  {
    IncludeEventsAtTime,
    SkipEventsAtTime,
  };

  enum class EventType
  {
    SustainPedal,
    NoteOff,
    NoteOn,
  };

  struct Event
  {
    double timeSeconds = 0.0;
    EventType type = EventType::NoteOn;
    int pitch = 0;
    int velocity = 0;
    bool sustainPedalDown = false;
  };

  struct RestorableNote
  {
    double startSeconds = 0.0;
    double endSeconds = 0.0;
    int pitch = 0;
    int velocity = 0;
  };

  void handleSustainPedal(const Event& event);
  void restoreSustainAt(double timeSeconds);
  void chaseHeldNotesAt(double timeSeconds);
  void resetPlaybackState();
  void resetCursor(double timeSeconds, CursorBoundary boundary);

  PianoSynth& m_synth;
  std::vector<Event> m_events;
  std::vector<RestorableNote> m_restorableNotes;
  std::size_t m_nextEventIndex = 0;
  bool m_sustainPedalDown = false;
  bool m_playbackPaused = false;
};
