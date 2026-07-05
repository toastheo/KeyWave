#include "audio/TimelineAudioScheduler.hpp"

#include "audio/PianoSynth.hpp"
#include "midi/MidiTimeline.hpp"

#include <algorithm>
#include <cstddef>
#include <cmath>
#include <functional>

TimelineAudioScheduler::TimelineAudioScheduler(PianoSynth& synth)
    : m_synth(synth)
{}

void TimelineAudioScheduler::setTimeline(const MidiTimeline& timeline)
{
  m_events.clear();
  m_events.reserve(timeline.notes().size() * 2);

  for (const auto& note : timeline.notes()) {
    m_events.push_back(Event{.timeSeconds = note.startSeconds,
                             .type = EventType::NoteOn,
                             .pitch = note.pitch,
                             .velocity = note.velocity});
    m_events.push_back(Event{.timeSeconds = note.startSeconds + note.durationSeconds,
                             .type = EventType::NoteOff,
                             .pitch = note.pitch});
  }

  std::ranges::sort(m_events, [](const Event& lhs, const Event& rhs) {
    if (lhs.timeSeconds != rhs.timeSeconds) {
      return lhs.timeSeconds < rhs.timeSeconds;
    }

    return lhs.type < rhs.type;
  });
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
    if (event.type == EventType::NoteOn) {
      m_synth.noteOn(PianoNote{.pitch = event.pitch, .velocity = event.velocity});
    }
    else {
      m_synth.noteOff(event.pitch);
    }
    ++m_nextEventIndex;
  }
}

void TimelineAudioScheduler::seek(double timeSeconds)
{
  m_synth.allNotesOff();
  resetCursor(std::max(0.0, timeSeconds), CursorBoundary::SkipEventsAtTime);
}

void TimelineAudioScheduler::stop()
{
  m_synth.allNotesOff();
  resetCursor(0.0, CursorBoundary::IncludeEventsAtTime);
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
