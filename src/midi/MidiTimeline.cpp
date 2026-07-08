#include "midi/MidiTimeline.hpp"

#include <algorithm>
#include <cmath>
#include <functional>
#include <iterator>
#include <vector>

#include "midi/MidiTypes.hpp"

void MidiTimeline::addNote(const Note& note)
{
  m_notes.push_back(note);
  m_lengthSeconds = std::max(m_lengthSeconds, note.startSeconds + note.durationSeconds);
}

const std::vector<Note>& MidiTimeline::notes() const
{
  return m_notes;
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

void MidiTimeline::addTempoEvent(const double timeSeconds, const double bpm)
{
  if (!std::isfinite(timeSeconds) || !std::isfinite(bpm) || bpm <= 0.0) {
    return;
  }

  m_tempoEvents.push_back(TempoEvent{
    .timeSeconds = std::max(0.0, timeSeconds),
    .bpm = bpm,
  });

  std::ranges::stable_sort(m_tempoEvents, {}, &TempoEvent::timeSeconds);
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
  const auto nextTempo = std::ranges::upper_bound(
    m_tempoEvents, clampedSeconds, std::ranges::less{}, &TempoEvent::timeSeconds);

  if (nextTempo == m_tempoEvents.begin()) {
    return defaultMidiBpm;
  }

  return std::prev(nextTempo)->bpm;
}
