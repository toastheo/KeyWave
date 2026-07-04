#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include "app/AppSettings.hpp"
#include "app/PlaybackTransportAction.hpp"
#include "app/PlaybackTransportControls.hpp"
#include "diagnostics/RecordingDiagnosticSink.hpp"
#include "input/Key.hpp"
#include "playback/PlaybackTransport.hpp"

namespace {

TEST_CASE("Playback transport controls map keys to transport actions", "[app][playback]")
{
  PlaybackTransport transport;
  RecordingDiagnosticSink diagnostics;
  constexpr PlaybackControlSettings settings;

  applyPlaybackTransportControl(Key::Space, transport, diagnostics, settings);
  CHECK(transport.state() == PlaybackState::Playing);

  applyPlaybackTransportControl(Key::Space, transport, diagnostics, settings);
  CHECK(transport.state() == PlaybackState::Paused);

  transport.seek(12.0);
  applyPlaybackTransportControl(Key::Left, transport, diagnostics, settings);
  CHECK(transport.currentTimeSeconds() == Catch::Approx(7.0));

  applyPlaybackTransportControl(Key::Right, transport, diagnostics, settings);
  CHECK(transport.currentTimeSeconds() == Catch::Approx(12.0));

  applyPlaybackTransportControl(Key::R, transport, diagnostics, settings);
  CHECK(transport.state() == PlaybackState::Playing);
  CHECK(transport.currentTimeSeconds() == Catch::Approx(0.0));

  transport.seek(3.0);
  applyPlaybackTransportControl(Key::S, transport, diagnostics, settings);
  CHECK(transport.state() == PlaybackState::Stopped);
  CHECK(transport.currentTimeSeconds() == Catch::Approx(0.0));
}

TEST_CASE("Playback transport controls clamp playback BPM", "[app][playback]")
{
  PlaybackTransport transport;
  RecordingDiagnosticSink diagnostics;
  constexpr PlaybackControlSettings settings;

  for (int count = 0; count < 100; ++count) {
    applyPlaybackTransportControl(Key::Up, transport, diagnostics, settings);
  }
  CHECK(transport.effectiveBpm(120.0) == Catch::Approx(300.0));

  for (int count = 0; count < 100; ++count) {
    applyPlaybackTransportControl(Key::Down, transport, diagnostics, settings);
  }
  CHECK(transport.effectiveBpm(120.0) == Catch::Approx(20.0));
}

TEST_CASE("Playback transport controls use custom playback control settings", "[app][playback]")
{
  PlaybackTransport transport;
  RecordingDiagnosticSink diagnostics;
  constexpr PlaybackControlSettings settings{
    .seekStepSeconds = 2.5,
    .minPlaybackBpm = 60.0,
    .maxPlaybackBpm = 180.0,
    .playbackBpmStep = 10.0,
  };

  transport.seek(12.0);
  applyPlaybackTransportControl(Key::Left, transport, diagnostics, settings);
  CHECK(transport.currentTimeSeconds() == Catch::Approx(9.5));

  applyPlaybackTransportControl(Key::Right, transport, diagnostics, settings);
  CHECK(transport.currentTimeSeconds() == Catch::Approx(12.0));

  for (int count = 0; count < 20; ++count) {
    applyPlaybackTransportControl(Key::Up, transport, diagnostics, settings);
  }
  CHECK(transport.effectiveBpm(120.0) == Catch::Approx(180.0));

  for (int count = 0; count < 20; ++count) {
    applyPlaybackTransportControl(Key::Down, transport, diagnostics, settings);
  }
  CHECK(transport.effectiveBpm(120.0) == Catch::Approx(60.0));
}

TEST_CASE("Playback transport controls ignore unmapped keys", "[app][playback]")
{
  PlaybackTransport transport;
  RecordingDiagnosticSink diagnostics;

  transport.play();
  transport.seek(13.0);
  transport.setPlaybackRate(1.5);

  applyPlaybackTransportControl(
    static_cast<Key>(255), transport, diagnostics, PlaybackControlSettings{});

  CHECK(transport.state() == PlaybackState::Playing);
  CHECK(transport.currentTimeSeconds() == Catch::Approx(13.0));
  CHECK(transport.playbackRate() == Catch::Approx(1.5));
  CHECK(diagnostics.messages.empty());
}

TEST_CASE("Playback transport actions can be shared by keyboard and UI controls", "[app][playback]")
{
  PlaybackTransport transport;
  constexpr PlaybackControlSettings settings;

  applyPlaybackTransportAction(PlaybackTransportAction::TogglePlayPause, transport, settings);
  CHECK(transport.state() == PlaybackState::Playing);

  applyPlaybackTransportAction(PlaybackTransportAction::TogglePlayPause, transport, settings);
  CHECK(transport.state() == PlaybackState::Paused);

  transport.seek(12.0);
  applyPlaybackTransportAction(PlaybackTransportAction::SeekBackward, transport, settings);
  CHECK(transport.currentTimeSeconds() == Catch::Approx(7.0));

  applyPlaybackTransportAction(PlaybackTransportAction::SeekForward, transport, settings);
  CHECK(transport.currentTimeSeconds() == Catch::Approx(12.0));

  applyPlaybackTransportAction(PlaybackTransportAction::Stop, transport, settings);
  CHECK(transport.state() == PlaybackState::Stopped);
  CHECK(transport.currentTimeSeconds() == Catch::Approx(0.0));
}

} // namespace
