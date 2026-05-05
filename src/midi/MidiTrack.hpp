#pragma once

#include "midi/MidiTypes.hpp"

#include <vector>

class MidiTrack {
public:
  explicit MidiTrack(int index = 0);

  void addNote(const Note& note);
  [[nodiscard]] int index() const;
  [[nodiscard]] const std::vector<Note>& notes() const;
  [[nodiscard]] bool empty() const;

private:
  int m_index = 0;
  std::vector<Note> m_notes;
};
