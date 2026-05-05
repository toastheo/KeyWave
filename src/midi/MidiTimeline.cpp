#include "midi/MidiTimeline.hpp"

#include <algorithm>
#include <utility>

void MidiTimeline::addNote(const Note& note) {
  m_notes.push_back(note);
  m_lengthSeconds = std::max(m_lengthSeconds, note.startSeconds + note.durationSeconds);

  if (note.track >= 0) {
    if (std::cmp_less_equal(m_tracks.size(), note.track)) {
      setTrackCount(note.track + 1);
    }

    m_tracks[static_cast<std::size_t>(note.track)].addNote(note);
  }
}

const std::vector<Note>& MidiTimeline::notes() const {
  return m_notes;
}

const std::vector<MidiTrack>& MidiTimeline::tracks() const {
  return m_tracks;
}

double MidiTimeline::lengthSeconds() const {
  return m_lengthSeconds;
}

bool MidiTimeline::empty() const {
  return m_notes.empty();
}

void MidiTimeline::setTrackCount(int trackCount) {
  if (trackCount < 0) {
    trackCount = 0;
  }

  const auto oldSize = m_tracks.size();
  m_tracks.resize(static_cast<std::size_t>(trackCount));

  for (auto index = oldSize; index < m_tracks.size(); ++index) {
    m_tracks[index] = MidiTrack(static_cast<int>(index));
  }
}

int MidiTimeline::trackCount() const {
  return static_cast<int>(m_tracks.size());
}

void MidiTimeline::setTicksPerQuarterNote(int ticksPerQuarterNote) {
  m_ticksPerQuarterNote = ticksPerQuarterNote;
}

int MidiTimeline::ticksPerQuarterNote() const {
  return m_ticksPerQuarterNote;
}
