#include "midi/MidiTrack.hpp"

#include <vector>

#include "midi/MidiTypes.hpp"

MidiTrack::MidiTrack(const int index)
    : m_index(index)
{}

void MidiTrack::addNote(const Note& note)
{
  m_notes.push_back(note);
}

int MidiTrack::index() const
{
  return m_index;
}

const std::vector<Note>& MidiTrack::notes() const
{
  return m_notes;
}

bool MidiTrack::empty() const
{
  return m_notes.empty();
}
