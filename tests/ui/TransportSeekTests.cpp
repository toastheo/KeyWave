#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <string>
#include <vector>

#include "audio/PianoSynth.hpp"
#include "audio/TimelineAudioScheduler.hpp"
#include "midi/MidiTimeline.hpp"
#include "playback/PlaybackTransport.hpp"
#include "ui/TransportSeek.hpp"

namespace {

class RecordingPianoSynth final : public PianoSynth
{
public:
  void noteOn(const PianoNote note) override
  {
    commands.push_back("on:" + std::to_string(note.pitch) + ":" +
                       std::to_string(note.velocity));
  }

  void noteOff(const int pitch) override
  {
    commands.push_back("off:" + std::to_string(pitch));
  }

  void setSustainPedal(const SustainPedalState state) override
  {
    commands.push_back(state == SustainPedalState::Down ? "sustain:down" : "sustain:up");
  }

  void setPlaybackPaused(const bool paused) override
  {
    commands.push_back(paused ? "playback:paused" : "playback:resumed");
  }

  void allNotesOff() override
  {
    commands.emplace_back("all-off");
  }

  std::vector<std::string> commands;
};

TEST_CASE("Transport seek preview stays silent and commit restores MIDI state", "[ui][audio]")
{
  RecordingPianoSynth synth;
  TimelineAudioScheduler scheduler(synth);
  MidiTimeline timeline;
  timeline.addSustainPedalEvent(SustainPedalEvent{.timeSeconds = 0.25, .pressed = true});
  timeline.addNote(Note{.pitch = 60, .velocity = 90, .startSeconds = 0.5, .durationSeconds = 2.0});
  scheduler.setTimeline(timeline);

  PlaybackTransport transport;
  previewTransportSeek(transport, 1.0, scheduler);

  CHECK(transport.currentTimeSeconds() == Catch::Approx(1.0));
  CHECK(synth.commands == std::vector<std::string>{"all-off"});

  commitTransportSeek(transport, scheduler);

  CHECK(synth.commands == std::vector<std::string>{
                            "all-off", "all-off", "sustain:down", "on:60:90"});
}

} // namespace
