#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <iostream>
#include <limits>
#include <sstream>

#include "diagnostics/RecordingDiagnosticSink.hpp"
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

TEST_CASE("PlaybackTransport does not print diagnostics directly", "[playback][diagnostics]")
{
  PlaybackTransport transport;
  std::ostringstream const capturedError;
  auto* const originalErrorBuffer = std::cerr.rdbuf(capturedError.rdbuf());

  transport.seek(std::numeric_limits<double>::infinity());
  transport.setPlaybackRate(-1.0);

  std::cerr.rdbuf(originalErrorBuffer);

  CHECK(capturedError.str().empty());
}

TEST_CASE("PlaybackTransport reports invalid timing input through diagnostics",
          "[playback][diagnostics]")
{
  RecordingDiagnosticSink diagnostics;
  PlaybackTransport transport(diagnostics);

  transport.seek(std::numeric_limits<double>::infinity());
  transport.setPlaybackRate(-1.0);

  REQUIRE(diagnostics.messages.size() == 2);
  CHECK(diagnostics.messages[0].severity == DiagnosticSeverity::Warning);
  CHECK(diagnostics.messages[0].message == "Playback seek ignored: time must be finite.");
  CHECK(diagnostics.messages[1].severity == DiagnosticSeverity::Warning);
  CHECK(diagnostics.messages[1].message ==
        "Playback rate ignored: rate must be a finite positive value.");
}

} // namespace
