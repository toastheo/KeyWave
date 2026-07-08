#pragma once

#include <vector>

#include "midi/MidiTypes.hpp"

class MidiTimeline
{
public:
  void addNote(const Note& note);
  [[nodiscard]] const std::vector<Note>& notes() const;
  [[nodiscard]] double lengthSeconds() const;
  [[nodiscard]] bool empty() const;
  [[nodiscard]] int minPitch() const;
  [[nodiscard]] int maxPitch() const;

  void addTempoEvent(double timeSeconds, double bpm);
  [[nodiscard]] const std::vector<TempoEvent>& tempoEvents() const;
  [[nodiscard]] double sourceBpmAt(double seconds) const;

private:
  std::vector<Note> m_notes;
  std::vector<TempoEvent> m_tempoEvents;
  double m_lengthSeconds = 0.0;
};
