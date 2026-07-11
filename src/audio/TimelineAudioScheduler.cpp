#include "audio/TimelineAudioScheduler.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <functional>

#include "audio/PianoSynth.hpp"
#include "midi/MidiTimeline.hpp"

TimelineAudioScheduler::TimelineAudioScheduler(PianoSynth& synth)
    : m_synth(synth)
{}

void TimelineAudioScheduler::setTimeline(const MidiTimeline& timeline)
{
  m_events.clear();
  m_events.reserve(timeline.notes().size() * 2 + timeline.sustainPedalEvents().size());

  for (const auto& note : timeline.notes()) {
    m_events.push_back(Event{.timeSeconds = note.startSeconds,
                             .type = EventType::NoteOn,
                             .pitch = note.pitch,
                             .velocity = note.velocity});
    m_events.push_back(Event{.timeSeconds = note.startSeconds + note.durationSeconds,
                             .type = EventType::NoteOff,
                             .pitch = note.pitch});
  }

  for (const auto& sustainPedalEvent : timeline.sustainPedalEvents()) {
    m_events.push_back(Event{.timeSeconds = sustainPedalEvent.timeSeconds,
                             .type = EventType::SustainPedal,
                             .sustainPedalDown = sustainPedalEvent.pressed});
  }

  std::ranges::sort(m_events, [](const Event& lhs, const Event& rhs) {
    if (lhs.timeSeconds != rhs.timeSeconds) {
      return lhs.timeSeconds < rhs.timeSeconds;
    }

    // Sustain changes run before same-tick note-offs so pedal-down can catch a release,
    // and pedal-up can release it before a same-tick retrigger.
    return lhs.type < rhs.type;
  });
  resetPlaybackState();
  resetCursor(0.0, CursorBoundary::IncludeEventsAtTime);
}

void TimelineAudioScheduler::update(const double previousTimeSeconds,
                                    const double currentTimeSeconds)
{
  if (!std::isfinite(previousTimeSeconds) || !std::isfinite(currentTimeSeconds) ||
      currentTimeSeconds < previousTimeSeconds) {
    return;
  }

  while (m_nextEventIndex < m_events.size() &&
         m_events[m_nextEventIndex].timeSeconds <= currentTimeSeconds) {
    const auto& event = m_events[m_nextEventIndex];
    switch (event.type) {
      case EventType::SustainPedal:
        handleSustainPedal(event);
        break;
      case EventType::NoteOff:
        m_synth.noteOff(event.pitch);
        break;
      case EventType::NoteOn:
        m_synth.noteOn(PianoNote{.pitch = event.pitch, .velocity = event.velocity});
        break;
    }
    ++m_nextEventIndex;
  }
}

void TimelineAudioScheduler::seek(double timeSeconds)
{
  resetPlaybackState();
  m_synth.allNotesOff();
  resetCursor(std::max(0.0, timeSeconds), CursorBoundary::SkipEventsAtTime);
}

void TimelineAudioScheduler::stop()
{
  resetPlaybackState();
  m_synth.allNotesOff();
  resetCursor(0.0, CursorBoundary::IncludeEventsAtTime);
}

void TimelineAudioScheduler::handleSustainPedal(const Event& event)
{
  if (event.sustainPedalDown == m_sustainPedalDown) {
    return;
  }

  m_sustainPedalDown = event.sustainPedalDown;
  m_synth.setSustainPedal(m_sustainPedalDown ? SustainPedalState::Down : SustainPedalState::Up);
}

void TimelineAudioScheduler::resetPlaybackState()
{
  if (m_sustainPedalDown) {
    m_synth.setSustainPedal(SustainPedalState::Up);
  }
  m_sustainPedalDown = false;
}

// Sets m_nextEventIndex to the index of the first event that comes after timeSeconds.
void TimelineAudioScheduler::resetCursor(const double timeSeconds, const CursorBoundary boundary)
{
  const auto cursor =
    boundary == CursorBoundary::IncludeEventsAtTime
      ? std::ranges::lower_bound(m_events, timeSeconds, std::ranges::less{}, &Event::timeSeconds)
      : std::ranges::upper_bound(m_events, timeSeconds, std::ranges::less{}, &Event::timeSeconds);

  m_nextEventIndex = static_cast<std::size_t>(cursor - m_events.begin());
}
