#include "midi/MidiFileLoader.hpp"
#include "midi/MidiFixtures.hpp"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <filesystem>

namespace {

constexpr double kTimeTolerance = 0.0001;

TEST_CASE("MidiFileLoader loads notes and applies tempo changes", "[midi]") {
  auto fixture = midi_fixtures::tempoChangeMidi();

  const auto timeline = MidiFileLoader::loadFromFile(fixture.path());

  REQUIRE(timeline.has_value());
  CHECK(timeline->trackCount() == 2);
  CHECK(timeline->ticksPerQuarterNote() == 480);
  REQUIRE(timeline->notes().size() == 2);
  CHECK(timeline->lengthSeconds() == Catch::Approx(1.25).margin(kTimeTolerance));

  const auto&[pitch, velocity, channel, track, startSeconds, durationSeconds] = timeline->notes()[0];
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

TEST_CASE("MidiFileLoader treats note-on velocity zero as note-off", "[midi]") {
  auto fixture = midi_fixtures::tempoChangeMidi();

  const auto timeline = MidiFileLoader::loadFromFile(fixture.path());

  REQUIRE(timeline.has_value());
  REQUIRE(timeline->notes().size() == 2);
  CHECK(timeline->notes()[1].pitch == 64);
  CHECK(timeline->notes()[1].durationSeconds == Catch::Approx(1.0).margin(kTimeTolerance));
}

TEST_CASE("MidiFileLoader pairs overlapping notes using FIFO order", "[midi]") {
  auto fixture = midi_fixtures::overlappingNotesMidi();

  const auto timeline = MidiFileLoader::loadFromFile(fixture.path());

  REQUIRE(timeline.has_value());
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

TEST_CASE("MidiFileLoader returns nullopt for missing files", "[midi]") {
  const auto missingPath = std::filesystem::temp_directory_path() / "keywave-missing-midi-file.mid";

  const auto timeline = MidiFileLoader::loadFromFile(missingPath);

  CHECK(!timeline.has_value());
}

} // namespace
