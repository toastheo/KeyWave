#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include "playback/PlaybackTransport.hpp"

namespace {

TEST_CASE("PlaybackTransport controls playback state and time", "[playback]")
{
  PlaybackTransport transport;

  CHECK(transport.state() == PlaybackState::Stopped);
  CHECK(transport.currentTimeSeconds() == Catch::Approx(0.0));
  CHECK(transport.playbackRate() == Catch::Approx(1.0));

  transport.play();
  transport.update(0.25);

  CHECK(transport.state() == PlaybackState::Playing);
  CHECK(transport.currentTimeSeconds() == Catch::Approx(0.25));

  transport.pause();
  transport.update(1.0);

  CHECK(transport.state() == PlaybackState::Paused);
  CHECK(transport.currentTimeSeconds() == Catch::Approx(0.25));

  transport.seek(2.0);
  transport.setPlaybackRate(2.0);
  transport.play();
  transport.update(0.5);

  CHECK(transport.currentTimeSeconds() == Catch::Approx(3.0));

  transport.stop();

  CHECK(transport.state() == PlaybackState::Stopped);
  CHECK(transport.currentTimeSeconds() == Catch::Approx(0.0));
}

TEST_CASE("PlaybackTransport ignores invalid timing input safely", "[playback]")
{
  PlaybackTransport transport;

  transport.seek(-4.0);
  CHECK(transport.currentTimeSeconds() == Catch::Approx(0.0));

  transport.play();
  transport.update(-1.0);

  CHECK(transport.currentTimeSeconds() == Catch::Approx(0.0));

  transport.setPlaybackRate(-2.0);
  transport.update(1.0);

  CHECK(transport.playbackRate() == Catch::Approx(1.0));
  CHECK(transport.currentTimeSeconds() == Catch::Approx(1.0));
}

} // namespace

