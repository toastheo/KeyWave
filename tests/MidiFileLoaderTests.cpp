#include "midi/MidiFileLoader.hpp"

#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

namespace {

void writeBytes(const std::filesystem::path& path, const std::vector<std::uint8_t>& bytes) {
  std::ofstream output(path, std::ios::binary);
  output.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
}

bool near(double actual, double expected) {
  return std::abs(actual - expected) < 0.0001;
}

bool expect(bool condition, const std::string& message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
    return false;
  }

  return true;
}

std::filesystem::path writeTempoMidiFixture() {
  const auto path = std::filesystem::temp_directory_path() / "keywave-midi-loader-test.mid";

  writeBytes(path, {
    'M', 'T', 'h', 'd',
    0x00, 0x00, 0x00, 0x06,
    0x00, 0x01,
    0x00, 0x02,
    0x01, 0xE0,

    'M', 'T', 'r', 'k',
    0x00, 0x00, 0x00, 0x13,
    0x00, 0xFF, 0x51, 0x03, 0x07, 0xA1, 0x20,
    0x81, 0x70, 0xFF, 0x51, 0x03, 0x0F, 0x42, 0x40,
    0x00, 0xFF, 0x2F, 0x00,

    'M', 'T', 'r', 'k',
    0x00, 0x00, 0x00, 0x16,
    0x00, 0x90, 0x3C, 0x40,
    0x81, 0x70, 0x80, 0x3C, 0x00,
    0x00, 0x90, 0x40, 0x50,
    0x83, 0x60, 0x90, 0x40, 0x00,
    0x00, 0xFF, 0x2F, 0x00,
  });

  return path;
}

} // namespace

int main() {
  bool passed = true;

  MidiFileLoader loader;
  const auto fixturePath = writeTempoMidiFixture();
  const auto timeline = loader.loadFromFile(fixturePath);

  passed &= expect(timeline.has_value(), "expected MIDI fixture to load");
  if (timeline) {
    const auto& notes = timeline->notes();

    passed &= expect(notes.size() == 2, "expected two parsed notes");
    passed &= expect(timeline->trackCount() == 2, "expected two MIDI tracks");
    passed &= expect(timeline->ticksPerQuarterNote() == 480, "expected TPQ 480");
    passed &= expect(near(timeline->lengthSeconds(), 1.25), "expected 1.25 second song length");

    if (notes.size() == 2) {
      passed &= expect(notes[0].pitch == 60, "expected first note pitch 60");
      passed &= expect(notes[0].velocity == 64, "expected first note velocity 64");
      passed &= expect(notes[0].channel == 0, "expected first note channel 0");
      passed &= expect(notes[0].track == 1, "expected first note on track 1");
      passed &= expect(near(notes[0].startSeconds, 0.0), "expected first note start at 0.0");
      passed &= expect(near(notes[0].durationSeconds, 0.25), "expected first note duration 0.25");

      passed &= expect(notes[1].pitch == 64, "expected second note pitch 64");
      passed &= expect(notes[1].velocity == 80, "expected second note velocity 80");
      passed &= expect(near(notes[1].startSeconds, 0.25), "expected second note start at 0.25");
      passed &= expect(near(notes[1].durationSeconds, 1.0), "expected second note duration 1.0");
    }
  }

  const auto missing = loader.loadFromFile(std::filesystem::temp_directory_path() / "does-not-exist.mid");
  passed &= expect(!missing.has_value(), "expected missing file to fail gracefully");

  std::filesystem::remove(fixturePath);
  return passed ? 0 : 1;
}
