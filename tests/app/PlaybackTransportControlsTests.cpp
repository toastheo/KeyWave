#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <string>
#include <vector>

#include "app/AppSettings.hpp"
#include "app/PlaybackTransportAction.hpp"
#include "app/PlaybackTransportControls.hpp"
#include "audio/PianoSynth.hpp"
#include "audio/TimelineAudioScheduler.hpp"
#include "diagnostics/RecordingDiagnosticSink.hpp"
#include "input/Key.hpp"
#include "midi/MidiTimeline.hpp"
#include "playback/PlaybackTransport.hpp"

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

  void setSustainPedal(const SustainPedalState state) override
  {
    commands.push_back(state == SustainPedalState::Down ? "sustain:down" : "sustain:up");
  }

  void allNotesOff() override
  {
    commands.emplace_back("all-off");
  }

  std::vector<std::string> commands;
};

struct AudioFixture
{
  RecordingPianoSynth synth;
  TimelineAudioScheduler scheduler{synth};
};

MidiTimeline makeTimelineWithTwoNotes()
{
  MidiTimeline timeline;
  timeline.addNote(Note{.pitch = 60, .velocity = 90, .startSeconds = 0.5, .durationSeconds = 1.5});
  timeline.addNote(Note{.pitch = 67, .velocity = 80, .startSeconds = 2.5, .durationSeconds = 0.5});
  return timeline;
}

TEST_CASE("Playback transport controls map keys to transport actions", "[app][playback]")
{
  AudioFixture audio;
  PlaybackTransport transport;
  RecordingDiagnosticSink diagnostics;
  constexpr PlaybackControlSettings settings;

  applyPlaybackTransportControl(Key::Space, transport, diagnostics, audio.scheduler, settings);
  CHECK(transport.state() == PlaybackState::Playing);

  applyPlaybackTransportControl(Key::Space, transport, diagnostics, audio.scheduler, settings);
  CHECK(transport.state() == PlaybackState::Paused);

  transport.seek(12.0);
  applyPlaybackTransportControl(Key::Left, transport, diagnostics, audio.scheduler, settings);
  CHECK(transport.currentTimeSeconds() == Catch::Approx(7.0));

  applyPlaybackTransportControl(Key::Right, transport, diagnostics, audio.scheduler, settings);
  CHECK(transport.currentTimeSeconds() == Catch::Approx(12.0));

  applyPlaybackTransportControl(Key::R, transport, diagnostics, audio.scheduler, settings);
  CHECK(transport.state() == PlaybackState::Playing);
  CHECK(transport.currentTimeSeconds() == Catch::Approx(0.0));

  transport.seek(3.0);
  applyPlaybackTransportControl(Key::S, transport, diagnostics, audio.scheduler, settings);
  CHECK(transport.state() == PlaybackState::Stopped);
  CHECK(transport.currentTimeSeconds() == Catch::Approx(0.0));
}

TEST_CASE("Playback transport actions clear audio on pause stop restart and seek",
          "[app][playback]")
{
  RecordingPianoSynth synth;
  TimelineAudioScheduler audioScheduler(synth);
  auto timeline = makeTimelineWithTwoNotes();
  audioScheduler.setTimeline(timeline);

  PlaybackTransport transport;
  constexpr PlaybackControlSettings settings{.seekStepSeconds = 1.0};

  transport.play();
  transport.update(1.0);
  audioScheduler.update(0.0, transport.currentTimeSeconds());
  REQUIRE(synth.commands == std::vector<std::string>{"on:60:90"});

  applyPlaybackTransportAction(
    PlaybackTransportAction::TogglePlayPause, transport, audioScheduler, settings, defaultMidiBpm);
  CHECK(transport.state() == PlaybackState::Paused);
  CHECK(synth.commands.back() == "all-off");

  applyPlaybackTransportAction(
    PlaybackTransportAction::TogglePlayPause, transport, audioScheduler, settings, defaultMidiBpm);
  CHECK(transport.state() == PlaybackState::Playing);
  CHECK(synth.commands.size() == 2);

  applyPlaybackTransportAction(
    PlaybackTransportAction::SeekForward, transport, audioScheduler, settings, defaultMidiBpm);
  CHECK(transport.currentTimeSeconds() == Catch::Approx(2.0));
  CHECK(synth.commands.back() == "all-off");

  audioScheduler.update(2.0, 2.5);
  CHECK(synth.commands.back() == "on:67:80");

  applyPlaybackTransportAction(
    PlaybackTransportAction::Restart, transport, audioScheduler, settings, defaultMidiBpm);
  CHECK(transport.currentTimeSeconds() == Catch::Approx(0.0));
  CHECK(transport.state() == PlaybackState::Playing);
  CHECK(synth.commands.back() == "all-off");

  applyPlaybackTransportAction(
    PlaybackTransportAction::Stop, transport, audioScheduler, settings, defaultMidiBpm);
  CHECK(transport.currentTimeSeconds() == Catch::Approx(0.0));
  CHECK(transport.state() == PlaybackState::Stopped);
  CHECK(synth.commands.back() == "all-off");
}

TEST_CASE("Playback transport controls clamp playback BPM", "[app][playback]")
{
  AudioFixture audio;
  PlaybackTransport transport;
  RecordingDiagnosticSink diagnostics;
  constexpr PlaybackControlSettings settings;

  for (int count = 0; count < 100; ++count) {
    applyPlaybackTransportControl(Key::Up, transport, diagnostics, audio.scheduler, settings);
  }
  CHECK(transport.effectiveBpm(120.0) == Catch::Approx(300.0));

  for (int count = 0; count < 100; ++count) {
    applyPlaybackTransportControl(Key::Down, transport, diagnostics, audio.scheduler, settings);
  }
  CHECK(transport.effectiveBpm(120.0) == Catch::Approx(20.0));
}

TEST_CASE("Playback transport controls use custom playback control settings", "[app][playback]")
{
  AudioFixture audio;
  PlaybackTransport transport;
  RecordingDiagnosticSink diagnostics;
  constexpr PlaybackControlSettings settings{
    .seekStepSeconds = 2.5,
    .minPlaybackBpm = 60.0,
    .maxPlaybackBpm = 180.0,
    .playbackBpmStep = 10.0,
  };

  transport.seek(12.0);
  applyPlaybackTransportControl(Key::Left, transport, diagnostics, audio.scheduler, settings);
  CHECK(transport.currentTimeSeconds() == Catch::Approx(9.5));

  applyPlaybackTransportControl(Key::Right, transport, diagnostics, audio.scheduler, settings);
  CHECK(transport.currentTimeSeconds() == Catch::Approx(12.0));

  for (int count = 0; count < 20; ++count) {
    applyPlaybackTransportControl(Key::Up, transport, diagnostics, audio.scheduler, settings);
  }
  CHECK(transport.effectiveBpm(120.0) == Catch::Approx(180.0));

  for (int count = 0; count < 20; ++count) {
    applyPlaybackTransportControl(Key::Down, transport, diagnostics, audio.scheduler, settings);
  }
  CHECK(transport.effectiveBpm(120.0) == Catch::Approx(60.0));
}

TEST_CASE("Playback transport controls ignore unmapped keys", "[app][playback]")
{
  AudioFixture audio;
  PlaybackTransport transport;
  RecordingDiagnosticSink diagnostics;

  transport.play();
  transport.seek(13.0);
  transport.setEffectiveBpm(120.0, 180.0);

  applyPlaybackTransportControl(
    static_cast<Key>(255), transport, diagnostics, audio.scheduler, PlaybackControlSettings{});

  CHECK(transport.state() == PlaybackState::Playing);
  CHECK(transport.currentTimeSeconds() == Catch::Approx(13.0));
  CHECK(transport.effectiveBpm(120.0) == Catch::Approx(180.0));
  CHECK(diagnostics.messages.empty());
}

TEST_CASE("Playback transport actions can be shared by keyboard and UI controls", "[app][playback]")
{
  AudioFixture audio;
  PlaybackTransport transport;
  constexpr PlaybackControlSettings settings;

  applyPlaybackTransportAction(
    PlaybackTransportAction::TogglePlayPause, transport, audio.scheduler, settings);
  CHECK(transport.state() == PlaybackState::Playing);

  applyPlaybackTransportAction(
    PlaybackTransportAction::TogglePlayPause, transport, audio.scheduler, settings);
  CHECK(transport.state() == PlaybackState::Paused);

  transport.seek(12.0);
  applyPlaybackTransportAction(
    PlaybackTransportAction::SeekBackward, transport, audio.scheduler, settings);
  CHECK(transport.currentTimeSeconds() == Catch::Approx(7.0));

  applyPlaybackTransportAction(
    PlaybackTransportAction::SeekForward, transport, audio.scheduler, settings);
  CHECK(transport.currentTimeSeconds() == Catch::Approx(12.0));

  applyPlaybackTransportAction(PlaybackTransportAction::Stop, transport, audio.scheduler, settings);
  CHECK(transport.state() == PlaybackState::Stopped);
  CHECK(transport.currentTimeSeconds() == Catch::Approx(0.0));
}

} // namespace
