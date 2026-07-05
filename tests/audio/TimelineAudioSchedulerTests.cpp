#include <catch2/catch_test_macros.hpp>
#include <string>
#include <vector>

#include "audio/PianoSynth.hpp"
#include "audio/TimelineAudioScheduler.hpp"
#include "midi/MidiTimeline.hpp"

namespace {

class RecordingPianoSynth final : public PianoSynth
{
public:
  void noteOn(const PianoNote note) override
  {
    commands.push_back("on:" + std::to_string(note.pitch) + ":" + std::to_string(note.velocity));
  }

  void noteOff(const int pitch) override
  {
    commands.push_back("off:" + std::to_string(pitch));
  }

  void allNotesOff() override
  {
    commands.emplace_back("all-off");
  }

  std::vector<std::string> commands;
};

MidiTimeline makeTimelineWithNote(const int pitch,
                                  const int velocity,
                                  const double startSeconds,
                                  const double durationSeconds)
{
  MidiTimeline timeline;
  timeline.addNote(Note{
    .pitch = pitch,
    .velocity = velocity,
    .startSeconds = startSeconds,
    .durationSeconds = durationSeconds,
  });
  return timeline;
}

TEST_CASE("TimelineAudioScheduler sends note-on when playback crosses note start", "[audio]")
{
  RecordingPianoSynth synth;
  TimelineAudioScheduler scheduler(synth);
  auto timeline = makeTimelineWithNote(60, 96, 0.5, 1.0);

  scheduler.setTimeline(timeline);
  scheduler.update(0.0, 0.5);
  scheduler.update(0.5, 1.0);

  REQUIRE(synth.commands.size() == 1);
  CHECK(synth.commands[0] == "on:60:96");
}

TEST_CASE("TimelineAudioScheduler sends note-off when playback crosses note end", "[audio]")
{
  RecordingPianoSynth synth;
  TimelineAudioScheduler scheduler(synth);
  auto timeline = makeTimelineWithNote(64, 72, 0.25, 0.75);

  scheduler.setTimeline(timeline);
  scheduler.update(0.0, 0.75);
  scheduler.update(0.75, 1.0);

  REQUIRE(synth.commands.size() == 2);
  CHECK(synth.commands[0] == "on:64:72");
  CHECK(synth.commands[1] == "off:64");
}

TEST_CASE("TimelineAudioScheduler seek skips old events and clears active notes", "[audio]")
{
  RecordingPianoSynth synth;
  TimelineAudioScheduler scheduler(synth);
  MidiTimeline timeline;
  timeline.addNote(Note{.pitch = 60, .velocity = 96, .startSeconds = 0.5, .durationSeconds = 1.5});
  timeline.addNote(Note{.pitch = 67, .velocity = 80, .startSeconds = 2.5, .durationSeconds = 0.5});

  scheduler.setTimeline(timeline);
  scheduler.update(0.0, 1.0);
  scheduler.seek(2.0);
  scheduler.update(2.0, 2.5);

  REQUIRE(synth.commands.size() == 3);
  CHECK(synth.commands[0] == "on:60:96");
  CHECK(synth.commands[1] == "all-off");
  CHECK(synth.commands[2] == "on:67:80");
}

TEST_CASE("TimelineAudioScheduler stop clears active notes and resets scheduling", "[audio]")
{
  RecordingPianoSynth synth;
  TimelineAudioScheduler scheduler(synth);
  auto timeline = makeTimelineWithNote(60, 96, 0.5, 1.0);

  scheduler.setTimeline(timeline);
  scheduler.update(0.0, 0.5);
  scheduler.stop();
  scheduler.update(0.0, 0.5);

  REQUIRE(synth.commands.size() == 3);
  CHECK(synth.commands[0] == "on:60:96");
  CHECK(synth.commands[1] == "all-off");
  CHECK(synth.commands[2] == "on:60:96");
}

} // namespace
