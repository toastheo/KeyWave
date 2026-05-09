#include "app/PlaybackTransportControls.hpp"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <sstream>

#include "input/Key.hpp"
#include "playback/PlaybackTransport.hpp"

namespace {

TEST_CASE("Playback transport controls map keys to transport actions", "[app][playback]")
{
  PlaybackTransport transport;
  std::ostringstream log;

  applyPlaybackTransportControl(Key::Space, transport, log);
  CHECK(transport.state() == PlaybackState::Playing);

  applyPlaybackTransportControl(Key::Space, transport, log);
  CHECK(transport.state() == PlaybackState::Paused);

  transport.seek(12.0);
  applyPlaybackTransportControl(Key::Left, transport, log);
  CHECK(transport.currentTimeSeconds() == Catch::Approx(7.0));

  applyPlaybackTransportControl(Key::Right, transport, log);
  CHECK(transport.currentTimeSeconds() == Catch::Approx(12.0));

  applyPlaybackTransportControl(Key::R, transport, log);
  CHECK(transport.state() == PlaybackState::Playing);
  CHECK(transport.currentTimeSeconds() == Catch::Approx(0.0));

  transport.seek(3.0);
  applyPlaybackTransportControl(Key::S, transport, log);
  CHECK(transport.state() == PlaybackState::Stopped);
  CHECK(transport.currentTimeSeconds() == Catch::Approx(0.0));
}

TEST_CASE("Playback transport controls clamp playback rate", "[app][playback]")
{
  PlaybackTransport transport;
  std::ostringstream log;

  for (int count = 0; count < 20; ++count) {
    applyPlaybackTransportControl(Key::Up, transport, log);
  }
  CHECK(transport.playbackRate() == Catch::Approx(4.0));

  for (int count = 0; count < 20; ++count) {
    applyPlaybackTransportControl(Key::Down, transport, log);
  }
  CHECK(transport.playbackRate() == Catch::Approx(0.25));
}

} // namespace
