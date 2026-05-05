#pragma once

#include "midi/MidiTrack.hpp"
#include "midi/MidiTypes.hpp"

#include <vector>

class MidiTimeline {
public:
  void addNote(const Note& note);
  [[nodiscard]] const std::vector<Note>& notes() const;
  [[nodiscard]] const std::vector<MidiTrack>& tracks() const;
  [[nodiscard]] double lengthSeconds() const;
  [[nodiscard]] bool empty() const;
  [[nodiscard]] int minPitch() const;
  [[nodiscard]] int maxPitch() const;

  void setTrackCount(int trackCount);
  [[nodiscard]] int trackCount() const;

  void setTicksPerQuarterNote(int ticksPerQuarterNote);
  [[nodiscard]] int ticksPerQuarterNote() const;

private:
  std::vector<Note> m_notes;
  std::vector<MidiTrack> m_tracks;
  double m_lengthSeconds = 0.0;
  int m_ticksPerQuarterNote = 0;
};
