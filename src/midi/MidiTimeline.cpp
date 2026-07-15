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
  m_lengthSeconds = std::max(m_lengthSeconds, audibleEndSeconds(note));
}

const std::vector<Note>& MidiTimeline::notes() const
{
  return m_notes;
}

double MidiTimeline::firstNoteStartSeconds() const
{
  double firstStartSeconds = 0.0;
  bool foundNote = false;
  for (const auto& note : m_notes) {
    if (!std::isfinite(note.startSeconds)) {
      continue;
    }

    if (!foundNote || note.startSeconds < firstStartSeconds) {
      firstStartSeconds = note.startSeconds;
      foundNote = true;
    }
  }
  return foundNote ? firstStartSeconds : 0.0;
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

void MidiTimeline::addSustainPedalEvent(const SustainPedalEvent& event)
{
  if (!std::isfinite(event.timeSeconds)) {
    return;
  }

  m_sustainPedalEvents.push_back(SustainPedalEvent{
    .timeSeconds = std::max(0.0, event.timeSeconds),
    .pressed = event.pressed,
    .channel = event.channel,
    .track = event.track,
  });

  std::ranges::stable_sort(m_sustainPedalEvents, {}, &SustainPedalEvent::timeSeconds);
  updateLength();
}

const std::vector<SustainPedalEvent>& MidiTimeline::sustainPedalEvents() const
{
  return m_sustainPedalEvents;
}

void MidiTimeline::updateLength()
{
  m_lengthSeconds = 0.0;

  for (const auto& note : m_notes) {
    m_lengthSeconds = std::max(m_lengthSeconds, audibleEndSeconds(note));
  }
}

double MidiTimeline::audibleEndSeconds(const Note& note) const
{
  const auto noteEndSeconds = note.startSeconds + note.durationSeconds;
  if (!std::isfinite(noteEndSeconds)) {
    return 0.0;
  }

  auto endSeconds = noteEndSeconds;
  bool sustainPressedAtNoteEnd = false;
  for (const auto& event : m_sustainPedalEvents) {
    if (event.channel != note.channel) {
      continue;
    }
    if (event.timeSeconds <= noteEndSeconds) {
      sustainPressedAtNoteEnd = event.pressed;
      continue;
    }

    if (!sustainPressedAtNoteEnd) {
      break;
    }
    if (!event.pressed) {
      endSeconds = event.timeSeconds;
      break;
    }
  }

  return endSeconds;
}
