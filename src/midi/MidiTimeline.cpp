#include "midi/MidiTimeline.hpp"

#include <algorithm>
#include <cmath>
#include <iterator>
#include <utility>

void MidiTimeline::addNote(const Note& note)
{
  m_notes.push_back(note);
  m_lengthSeconds = std::max(m_lengthSeconds, note.startSeconds + note.durationSeconds);

  if (note.track >= 0) {
    if (std::cmp_less_equal(m_tracks.size(), note.track)) {
      setTrackCount(note.track + 1);
    }

    m_tracks[static_cast<std::size_t>(note.track)].addNote(note);
  }
}

const std::vector<Note>& MidiTimeline::notes() const
{
  return m_notes;
}

const std::vector<MidiTrack>& MidiTimeline::tracks() const
{
  return m_tracks;
}

double MidiTimeline::lengthSeconds() const
{
  return m_lengthSeconds;
}

bool MidiTimeline::empty() const
{
  return m_notes.empty();
}

int MidiTimeline::minPitch() const
{
  if (m_notes.empty()) {
    return 0;
  }

  const auto minNote = std::ranges::min_element(m_notes, [](const Note& left, const Note& right) {
    return left.pitch < right.pitch;
  });

  return minNote->pitch;
}

int MidiTimeline::maxPitch() const
{
  if (m_notes.empty()) {
    return 0;
  }

  const auto maxNote = std::ranges::max_element(m_notes, [](const Note& left, const Note& right) {
    return left.pitch < right.pitch;
  });

  return maxNote->pitch;
}

void MidiTimeline::setTrackCount(int trackCount)
{
  if (trackCount < 0) {
    trackCount = 0;
  }

  const auto oldSize = m_tracks.size();
  m_tracks.resize(static_cast<std::size_t>(trackCount));

  for (auto index = oldSize; index < m_tracks.size(); ++index) {
    m_tracks[index] = MidiTrack(static_cast<int>(index));
  }
}

int MidiTimeline::trackCount() const
{
  return static_cast<int>(m_tracks.size());
}

void MidiTimeline::setTicksPerQuarterNote(int ticksPerQuarterNote)
{
  m_ticksPerQuarterNote = ticksPerQuarterNote;
}

int MidiTimeline::ticksPerQuarterNote() const
{
  return m_ticksPerQuarterNote;
}

void MidiTimeline::addTempoEvent(const double timeSeconds, const double bpm)
{
  if (!std::isfinite(timeSeconds) || !std::isfinite(bpm) || bpm <= 0.0) {
    return;
  }

  m_tempoEvents.push_back(TempoEvent{
    .timeSeconds = std::max(0.0, timeSeconds),
    .bpm = bpm,
  });

  std::stable_sort(m_tempoEvents.begin(),
                   m_tempoEvents.end(),
                   [](const TempoEvent& left, const TempoEvent& right) {
                     return left.timeSeconds < right.timeSeconds;
                   });
}

const std::vector<TempoEvent>& MidiTimeline::tempoEvents() const
{
  return m_tempoEvents;
}

double MidiTimeline::sourceBpmAt(const double seconds) const
{
  if (m_tempoEvents.empty()) {
    return defaultMidiBpm;
  }

  const auto clampedSeconds = std::isfinite(seconds) ? std::max(0.0, seconds) : 0.0;
  const auto nextTempo = std::upper_bound(m_tempoEvents.begin(),
                                          m_tempoEvents.end(),
                                          clampedSeconds,
                                          [](const double timeSeconds, const TempoEvent& event) {
                                            return timeSeconds < event.timeSeconds;
                                          });

  if (nextTempo == m_tempoEvents.begin()) {
    return defaultMidiBpm;
  }

  return std::prev(nextTempo)->bpm;
}
