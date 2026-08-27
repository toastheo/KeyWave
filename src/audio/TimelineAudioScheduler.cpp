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

void TimelineAudioScheduler::setTimeline(const MidiTimeline& timeline,
                                         const double timelineOffsetSeconds)
{
  const auto safeOffsetSeconds = std::isfinite(timelineOffsetSeconds) ? timelineOffsetSeconds : 0.0;
  m_events.clear();
  m_events.reserve(timeline.notes().size() * 2 + timeline.sustainPedalEvents().size());
  m_restorableNotes.clear();
  m_restorableNotes.reserve(timeline.notes().size());

  for (const auto& note : timeline.notes()) {
    const auto startSeconds = note.startSeconds - safeOffsetSeconds;
    const auto endSeconds = startSeconds + note.durationSeconds;

    if (std::isfinite(startSeconds) && std::isfinite(endSeconds) && note.durationSeconds > 0.0) {
      m_restorableNotes.push_back(RestorableNote{
        .startSeconds = startSeconds,
        .endSeconds = endSeconds,
        .pitch = note.pitch,
        .velocity = note.velocity,
      });
    }

    m_events.push_back(Event{.timeSeconds = startSeconds,
                             .type = EventType::NoteOn,
                             .pitch = note.pitch,
                             .velocity = note.velocity});
    m_events.push_back(
      Event{.timeSeconds = endSeconds, .type = EventType::NoteOff, .pitch = note.pitch});
  }

  for (const auto& sustainPedalEvent : timeline.sustainPedalEvents()) {
    m_events.push_back(Event{.timeSeconds = sustainPedalEvent.timeSeconds - safeOffsetSeconds,
                             .type = EventType::SustainPedal,
                             .sustainPedalDown = sustainPedalEvent.pressed});
  }

  std::ranges::stable_sort(m_events, [](const Event& lhs, const Event& rhs) {
    if (lhs.timeSeconds != rhs.timeSeconds) {
      return lhs.timeSeconds < rhs.timeSeconds;
    }

    // Sustain changes run before same-tick note-offs so pedal-down can catch a release,
    // and pedal-up can release it before a same-tick retrigger.
    return lhs.type < rhs.type;
  });
  std::ranges::sort(m_restorableNotes, [](const RestorableNote& lhs, const RestorableNote& rhs) {
    if (lhs.startSeconds != rhs.startSeconds) {
      return lhs.startSeconds < rhs.startSeconds;
    }
    return lhs.pitch < rhs.pitch;
  });
  resetPlaybackState();
  resetCursor(0.0, CursorBoundary::IncludeEventsAtTime);
  resume();
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

void TimelineAudioScheduler::pause()
{
  if (m_playbackPaused) {
    return;
  }

  m_playbackPaused = true;
  m_synth.setPlaybackPaused(true);
}

void TimelineAudioScheduler::resume()
{
  if (!m_playbackPaused) {
    return;
  }

  m_playbackPaused = false;
  m_synth.setPlaybackPaused(false);
}

void TimelineAudioScheduler::seek(const double timeSeconds, const SeekMode mode)
{
  const auto safeTimeSeconds = std::isfinite(timeSeconds) ? std::max(0.0, timeSeconds) : 0.0;

  resetPlaybackState();
  m_synth.allNotesOff();
  resetCursor(safeTimeSeconds, CursorBoundary::SkipEventsAtTime);

  if (mode == SeekMode::RestoreState) {
    restoreSustainAt(safeTimeSeconds);
    chaseHeldNotesAt(safeTimeSeconds);
  }
}

void TimelineAudioScheduler::stop()
{
  resetPlaybackState();
  m_synth.allNotesOff();
  resetCursor(0.0, CursorBoundary::IncludeEventsAtTime);
  resume();
}

void TimelineAudioScheduler::handleSustainPedal(const Event& event)
{
  if (event.sustainPedalDown == m_sustainPedalDown) {
    return;
  }

  m_sustainPedalDown = event.sustainPedalDown;
  m_synth.setSustainPedal(m_sustainPedalDown ? SustainPedalState::Down : SustainPedalState::Up);
}

void TimelineAudioScheduler::restoreSustainAt(const double timeSeconds)
{
  bool sustainPedalDown = false;
  for (const auto& event : m_events) {
    if (event.timeSeconds > timeSeconds) {
      break;
    }
    if (event.type == EventType::SustainPedal) {
      sustainPedalDown = event.sustainPedalDown;
    }
  }

  if (sustainPedalDown) {
    handleSustainPedal(Event{.type = EventType::SustainPedal, .sustainPedalDown = true});
  }
}

void TimelineAudioScheduler::chaseHeldNotesAt(const double timeSeconds)
{
  for (const auto& note : m_restorableNotes) {
    if (note.startSeconds > timeSeconds) {
      break;
    }
    if (note.endSeconds > timeSeconds) {
      m_synth.noteOn(PianoNote{.pitch = note.pitch, .velocity = note.velocity});
    }
  }
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
