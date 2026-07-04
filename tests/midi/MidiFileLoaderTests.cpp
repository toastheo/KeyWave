#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>

#include "diagnostics/RecordingDiagnosticSink.hpp"
#include "midi/MidiFileLoader.hpp"
#include "midi/MidiFixtures.hpp"

namespace {

constexpr double kTimeTolerance = 0.0001;

std::filesystem::path nextUnsupportedFilePath()
{
  const auto now = std::chrono::steady_clock::now().time_since_epoch().count();
  return std::filesystem::temp_directory_path() /
         ("keywave-unsupported-midi-test-" + std::to_string(now) + ".png");
}

TEST_CASE("MidiFileLoader loads notes and applies tempo changes", "[midi]")
{
  auto fixture = midi_fixtures::tempoChangeMidi();

  const auto timeline = MidiFileLoader::loadFromFile(fixture.path());

  REQUIRE(timeline.has_value());
  CHECK(timeline->trackCount() == 2);
  CHECK(timeline->ticksPerQuarterNote() == 480);
  CHECK(timeline->sourceBpmAt(0.0) == Catch::Approx(120.0));
  CHECK(timeline->sourceBpmAt(0.249) == Catch::Approx(120.0));
  CHECK(timeline->sourceBpmAt(0.25) == Catch::Approx(60.0));
  CHECK(timeline->sourceBpmAt(12.0) == Catch::Approx(60.0));
  REQUIRE(timeline->notes().size() == 2);
  CHECK(timeline->lengthSeconds() == Catch::Approx(1.25).margin(kTimeTolerance));

  const auto& [pitch, velocity, channel, track, startSeconds, durationSeconds] =
    timeline->notes()[0];
  CHECK(pitch == 60);
  CHECK(velocity == 64);
  CHECK(channel == 0);
  CHECK(track == 1);
  CHECK(startSeconds == Catch::Approx(0.0).margin(kTimeTolerance));
  CHECK(durationSeconds == Catch::Approx(0.25).margin(kTimeTolerance));

  const auto& second = timeline->notes()[1];
  CHECK(second.pitch == 64);
  CHECK(second.velocity == 80);
  CHECK(second.channel == 0);
  CHECK(second.track == 1);
  CHECK(second.startSeconds == Catch::Approx(0.25).margin(kTimeTolerance));
  CHECK(second.durationSeconds == Catch::Approx(1.0).margin(kTimeTolerance));
}

TEST_CASE("MidiFileLoader treats note-on velocity zero as note-off", "[midi]")
{
  auto fixture = midi_fixtures::tempoChangeMidi();

  const auto timeline = MidiFileLoader::loadFromFile(fixture.path());

  REQUIRE(timeline.has_value());
  REQUIRE(timeline->notes().size() == 2);
  CHECK(timeline->notes()[1].pitch == 64);
  CHECK(timeline->notes()[1].durationSeconds == Catch::Approx(1.0).margin(kTimeTolerance));
}

TEST_CASE("MidiFileLoader pairs overlapping notes using FIFO order", "[midi]")
{
  auto fixture = midi_fixtures::overlappingNotesMidi();

  const auto timeline = MidiFileLoader::loadFromFile(fixture.path());

  REQUIRE(timeline.has_value());
  CHECK(timeline->sourceBpmAt(0.0) == Catch::Approx(120.0));
  CHECK(timeline->sourceBpmAt(2.0) == Catch::Approx(120.0));
  REQUIRE(timeline->notes().size() == 2);

  const auto& first = timeline->notes()[0];
  CHECK(first.pitch == 60);
  CHECK(first.velocity == 40);
  CHECK(first.startSeconds == Catch::Approx(0.0).margin(kTimeTolerance));
  CHECK(first.durationSeconds == Catch::Approx(0.25).margin(kTimeTolerance));

  const auto& second = timeline->notes()[1];
  CHECK(second.pitch == 60);
  CHECK(second.velocity == 50);
  CHECK(second.startSeconds == Catch::Approx(0.125).margin(kTimeTolerance));
  CHECK(second.durationSeconds == Catch::Approx(0.25).margin(kTimeTolerance));
}

TEST_CASE("MidiFileLoader returns nullopt for missing files", "[midi]")
{
  const auto missingPath = std::filesystem::temp_directory_path() / "keywave-missing-midi-file.mid";

  const auto timeline = MidiFileLoader::loadFromFile(missingPath);

  CHECK(!timeline.has_value());
}

TEST_CASE("MidiFileLoader returns nullopt and reports an error for unsupported file contents",
          "[midi]")
{
  const auto unsupportedPath = nextUnsupportedFilePath();
  {
    std::ofstream output(unsupportedPath, std::ios::binary);
    output << "not a midi file";
  }

  RecordingDiagnosticSink diagnostics;
  std::ostringstream capturedStderr;
  auto* const originalStderr = std::cerr.rdbuf(capturedStderr.rdbuf());
  const auto timeline = MidiFileLoader::loadFromFile(unsupportedPath, diagnostics);
  std::cerr.rdbuf(originalStderr);

  CHECK(!timeline.has_value());
  CHECK(capturedStderr.str().empty());
  REQUIRE_FALSE(diagnostics.messages.empty());
  CHECK(diagnostics.messages.back().severity == DiagnosticSeverity::Error);
  CHECK(diagnostics.messages.back().message.find("could not parse file") != std::string::npos);

  std::filesystem::remove(unsupportedPath);
}

} // namespace
