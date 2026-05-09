#include "midi/MidiTimeline.hpp"
#include "midi/MidiTimelineQuery.hpp"

#include <catch2/catch_test_macros.hpp>

namespace {

MidiTimeline makeTimeline() {
  MidiTimeline timeline;
  timeline.addNote(Note{.pitch = 60, .velocity = 90, .channel = 0, .track = 2, .startSeconds = 1.0, .durationSeconds = 1.0});
  timeline.addNote(Note{.pitch = 64, .velocity = 80, .channel = 1, .track = 1, .startSeconds = 0.5, .durationSeconds = 0.75});
  timeline.addNote(Note{.pitch = 65, .velocity = 70, .channel = 2, .track = 0, .startSeconds = 3.0, .durationSeconds = 1.0});
  timeline.addNote(Note{.pitch = 21, .velocity = 60, .channel = 0, .track = 0, .startSeconds = 0.0, .durationSeconds = 4.0});
  timeline.addNote(Note{.pitch = 109, .velocity = 60, .channel = 0, .track = 0, .startSeconds = 1.0, .durationSeconds = 1.0});
  return timeline;
}

TEST_CASE("MidiTimelineQuery finds notes overlapping time and pitch ranges", "[midi][query]") {
  const auto timeline = makeTimeline();
  const MidiTimelineQuery query(timeline);

  const auto notes = query.findNotes(TimelineViewport{
    .timeRange = TimeRange{.startSeconds = 0.75, .endSeconds = 2.5},
    .pitchRange = PitchRange{.minPitch = 21, .maxPitch = 108},
  });

  REQUIRE(notes.size() == 3);
  CHECK(notes[0].note.pitch == 21);
  CHECK(notes[0].startsBeforeRange);
  CHECK(notes[0].endsAfterRange);
  CHECK(notes[1].note.pitch == 64);
  CHECK(notes[1].startsBeforeRange);
  CHECK_FALSE(notes[1].endsAfterRange);
  CHECK(notes[2].note.pitch == 60);
  CHECK_FALSE(notes[2].startsBeforeRange);
  CHECK_FALSE(notes[2].endsAfterRange);
}

TEST_CASE("MidiTimelineQuery sorts returned notes deterministically", "[midi][query]") {
  MidiTimeline timeline;
  timeline.addNote(Note{.pitch = 62, .channel = 1, .track = 2, .startSeconds = 1.0, .durationSeconds = 1.0});
  timeline.addNote(Note{.pitch = 60, .channel = 1, .track = 2, .startSeconds = 1.0, .durationSeconds = 1.0});
  timeline.addNote(Note{.pitch = 60, .channel = 0, .track = 2, .startSeconds = 1.0, .durationSeconds = 1.0});
  timeline.addNote(Note{.pitch = 60, .channel = 0, .track = 1, .startSeconds = 1.0, .durationSeconds = 1.0});
  timeline.addNote(Note{.pitch = 60, .channel = 0, .track = 0, .startSeconds = 0.5, .durationSeconds = 1.0});

  const MidiTimelineQuery query(timeline);
  const auto notes = query.findNotes(TimelineViewport{
    .timeRange = TimeRange{.startSeconds = 0.0, .endSeconds = 3.0},
    .pitchRange = PitchRange{.minPitch = 0, .maxPitch = 127},
  });

  REQUIRE(notes.size() == 5);
  CHECK(notes[0].note.startSeconds == 0.5);
  CHECK(notes[1].note.pitch == 60);
  CHECK(notes[1].note.channel == 0);
  CHECK(notes[1].note.track == 1);
  CHECK(notes[2].note.pitch == 60);
  CHECK(notes[2].note.channel == 0);
  CHECK(notes[2].note.track == 2);
  CHECK(notes[3].note.pitch == 60);
  CHECK(notes[3].note.channel == 1);
  CHECK(notes[4].note.pitch == 62);
}

TEST_CASE("MidiTimelineQuery finds notes active at playback time", "[midi][query]") {
  MidiTimeline timeline;
  timeline.addNote(Note{.pitch = 67, .velocity = 80, .channel = 1, .track = 2, .startSeconds = 1.0, .durationSeconds = 2.0});
  timeline.addNote(Note{.pitch = 60, .velocity = 90, .channel = 0, .track = 1, .startSeconds = 2.0, .durationSeconds = 1.0});
  timeline.addNote(Note{.pitch = 60, .velocity = 70, .channel = 0, .track = 0, .startSeconds = 2.0, .durationSeconds = 1.0});
  timeline.addNote(Note{.pitch = 64, .velocity = 50, .channel = 0, .track = 0, .startSeconds = 3.0, .durationSeconds = 1.0});
  timeline.addNote(Note{.pitch = 65, .velocity = 50, .channel = 0, .track = 0, .startSeconds = 2.0, .durationSeconds = 0.0});
  timeline.addNote(Note{.pitch = 66, .velocity = 50, .channel = 0, .track = 0, .startSeconds = 2.0, .durationSeconds = -1.0});

  const MidiTimelineQuery query(timeline);

  const auto activeNotes = query.findActiveNotesAt(2.0);

  REQUIRE(activeNotes.size() == 3);
  CHECK(activeNotes[0].pitch == 60);
  CHECK(activeNotes[0].channel == 0);
  CHECK(activeNotes[0].track == 0);
  CHECK(activeNotes[1].pitch == 60);
  CHECK(activeNotes[1].channel == 0);
  CHECK(activeNotes[1].track == 1);
  CHECK(activeNotes[2].pitch == 67);
}

TEST_CASE("MidiTimelineQuery active note end is exclusive", "[midi][query]") {
  MidiTimeline timeline;
  timeline.addNote(Note{.pitch = 60, .velocity = 90, .startSeconds = 1.0, .durationSeconds = 1.0});

  const MidiTimelineQuery query(timeline);

  CHECK(query.findActiveNotesAt(0.999).empty());
  CHECK_FALSE(query.findActiveNotesAt(1.0).empty());
  CHECK(query.findActiveNotesAt(2.0).empty());
  CHECK(query.findActiveNotesAt(-0.1).empty());
}

TEST_CASE("MidiTimelineQuery time-only and pitch-only helpers use inclusive filters", "[midi][query]") {
  const auto timeline = makeTimeline();
  const MidiTimelineQuery query(timeline);

  const auto timeNotes = query.findNotesInTimeRange(TimeRange{.startSeconds = 0.75, .endSeconds = 1.0});
  CHECK(timeNotes.size() == 2);

  const auto pitchNotes = query.findNotesInPitchRange(PitchRange{.minPitch = 60, .maxPitch = 65});
  REQUIRE(pitchNotes.size() == 3);
  CHECK(pitchNotes[0].note.pitch == 64);
  CHECK(pitchNotes[1].note.pitch == 60);
  CHECK(pitchNotes[2].note.pitch == 65);
}

TEST_CASE("MidiTimelineQuery returns empty results for invalid ranges", "[midi][query]") {
  const auto timeline = makeTimeline();
  const MidiTimelineQuery query(timeline);

  CHECK(query.findNotes(TimelineViewport{
    .timeRange = TimeRange{.startSeconds = 2.0, .endSeconds = 2.0},
    .pitchRange = PitchRange{.minPitch = 21, .maxPitch = 108},
  }).empty());

  CHECK(query.findNotes(TimelineViewport{
    .timeRange = TimeRange{.startSeconds = 0.0, .endSeconds = 2.0},
    .pitchRange = PitchRange{.minPitch = 108, .maxPitch = 21},
  }).empty());
}

TEST_CASE("MidiTimeline exposes safe pitch helpers for empty and populated timelines", "[midi][query]") {
  MidiTimeline const emptyTimeline;
  CHECK(emptyTimeline.empty());
  CHECK(emptyTimeline.minPitch() == 0);
  CHECK(emptyTimeline.maxPitch() == 0);

  const auto timeline = makeTimeline();
  CHECK_FALSE(timeline.empty());
  CHECK(timeline.minPitch() == 21);
  CHECK(timeline.maxPitch() == 109);
}

} // namespace
