#include "midi/MidiFixtures.hpp"

#include <chrono>
#include <fstream>
#include <utility>

namespace midi_fixtures {
namespace {

std::filesystem::path nextMidiPath()
{
  const auto now = std::chrono::steady_clock::now().time_since_epoch().count();
  return std::filesystem::temp_directory_path() /
         ("keywave-midi-test-" + std::to_string(now) + ".mid");
}

void writeBytes(const std::filesystem::path& path, const std::vector<std::uint8_t>& bytes)
{
  std::ofstream output(path, std::ios::binary);
  output.write(reinterpret_cast<const char*>(bytes.data()),
               static_cast<std::streamsize>(bytes.size()));
}

} // namespace

TemporaryMidiFile::TemporaryMidiFile(const std::vector<std::uint8_t>& bytes)
    : m_path(nextMidiPath())
{
  writeBytes(m_path, bytes);
}

TemporaryMidiFile::~TemporaryMidiFile()
{
  if (!m_path.empty()) {
    std::filesystem::remove(m_path);
  }
}

TemporaryMidiFile::TemporaryMidiFile(TemporaryMidiFile&& other) noexcept
    : m_path(std::exchange(other.m_path, {}))
{}

TemporaryMidiFile& TemporaryMidiFile::operator=(TemporaryMidiFile&& other) noexcept
{
  if (this != &other) {
    if (!m_path.empty()) {
      std::filesystem::remove(m_path);
    }

    m_path = std::exchange(other.m_path, {});
  }

  return *this;
}

const std::filesystem::path& TemporaryMidiFile::path() const
{
  return m_path;
}

TemporaryMidiFile tempoChangeMidi()
{
  return TemporaryMidiFile({
      'M',  'T',  'h',  'd',  0x00, 0x00, 0x00, 0x06, 0x00, 0x01, 0x00, 0x02, 0x01, 0xE0,

      'M',  'T',  'r',  'k',  0x00, 0x00, 0x00, 0x13, 0x00, 0xFF, 0x51, 0x03, 0x07, 0xA1, 0x20,
      0x81, 0x70, 0xFF, 0x51, 0x03, 0x0F, 0x42, 0x40, 0x00, 0xFF, 0x2F, 0x00,

      'M',  'T',  'r',  'k',  0x00, 0x00, 0x00, 0x16, 0x00, 0x90, 0x3C, 0x40, 0x81, 0x70, 0x80,
      0x3C, 0x00, 0x00, 0x90, 0x40, 0x50, 0x83, 0x60, 0x90, 0x40, 0x00, 0x00, 0xFF, 0x2F, 0x00,
  });
}

TemporaryMidiFile overlappingNotesMidi()
{
  return TemporaryMidiFile({
      'M',  'T',  'h',  'd',  0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x01, 0x01, 0xE0,

      'M',  'T',  'r',  'k',  0x00, 0x00, 0x00, 0x1A, 0x00, 0x90, 0x3C, 0x28, 0x78, 0x90,
      0x3C, 0x32, 0x78, 0x80, 0x3C, 0x00, 0x78, 0x80, 0x3C, 0x00, 0x00, 0xFF, 0x2F, 0x00,
  });
}

} // namespace midi_fixtures
