#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <sstream>

#include "app/AppSettings.hpp"
#include "app/PlaybackTransportAction.hpp"
#include "app/PlaybackTransportControls.hpp"
#include "input/Key.hpp"
#include "playback/PlaybackTransport.hpp"

namespace {

TEST_CASE("Playback transport controls map keys to transport actions", "[app][playback]")
{
  PlaybackTransport transport;
  std::ostringstream log;
  constexpr PlaybackControlSettings settings;

  applyPlaybackTransportControl(Key::Space, transport, log, settings);
  CHECK(transport.state() == PlaybackState::Playing);

  applyPlaybackTransportControl(Key::Space, transport, log, settings);
  CHECK(transport.state() == PlaybackState::Paused);

  transport.seek(12.0);
  applyPlaybackTransportControl(Key::Left, transport, log, settings);
  CHECK(transport.currentTimeSeconds() == Catch::Approx(7.0));

  applyPlaybackTransportControl(Key::Right, transport, log, settings);
  CHECK(transport.currentTimeSeconds() == Catch::Approx(12.0));

  applyPlaybackTransportControl(Key::R, transport, log, settings);
  CHECK(transport.state() == PlaybackState::Playing);
  CHECK(transport.currentTimeSeconds() == Catch::Approx(0.0));

  transport.seek(3.0);
  applyPlaybackTransportControl(Key::S, transport, log, settings);
  CHECK(transport.state() == PlaybackState::Stopped);
  CHECK(transport.currentTimeSeconds() == Catch::Approx(0.0));
}

TEST_CASE("Playback transport controls clamp playback rate", "[app][playback]")
{
  PlaybackTransport transport;
  std::ostringstream log;
  constexpr PlaybackControlSettings settings;

  for (int count = 0; count < 20; ++count) {
    applyPlaybackTransportControl(Key::Up, transport, log, settings);
  }
  CHECK(transport.playbackRate() == Catch::Approx(4.0));

  for (int count = 0; count < 20; ++count) {
    applyPlaybackTransportControl(Key::Down, transport, log, settings);
  }
  CHECK(transport.playbackRate() == Catch::Approx(0.25));
}

TEST_CASE("Playback transport controls use custom playback control settings", "[app][playback]")
{
  PlaybackTransport transport;
  std::ostringstream log;
  constexpr PlaybackControlSettings settings{
    .seekStepSeconds = 2.5,
    .minPlaybackRate = 0.5,
    .maxPlaybackRate = 2.0,
    .playbackRateStep = 0.5,
  };

  transport.seek(12.0);
  applyPlaybackTransportControl(Key::Left, transport, log, settings);
  CHECK(transport.currentTimeSeconds() == Catch::Approx(9.5));

  applyPlaybackTransportControl(Key::Right, transport, log, settings);
  CHECK(transport.currentTimeSeconds() == Catch::Approx(12.0));

  for (int count = 0; count < 20; ++count) {
    applyPlaybackTransportControl(Key::Up, transport, log, settings);
  }
  CHECK(transport.playbackRate() == Catch::Approx(2.0));

  for (int count = 0; count < 20; ++count) {
    applyPlaybackTransportControl(Key::Down, transport, log, settings);
  }
  CHECK(transport.playbackRate() == Catch::Approx(0.5));
}

TEST_CASE("Playback transport controls ignore unmapped keys", "[app][playback]")
{
  PlaybackTransport transport;
  std::ostringstream log;

  transport.play();
  transport.seek(13.0);
  transport.setPlaybackRate(1.5);

  applyPlaybackTransportControl(static_cast<Key>(255), transport, log, PlaybackControlSettings{});

  CHECK(transport.state() == PlaybackState::Playing);
  CHECK(transport.currentTimeSeconds() == Catch::Approx(13.0));
  CHECK(transport.playbackRate() == Catch::Approx(1.5));
  CHECK(log.str().empty());
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
