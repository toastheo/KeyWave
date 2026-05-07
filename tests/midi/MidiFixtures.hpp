#pragma once

#include <filesystem>
#include <vector>

namespace midi_fixtures {

class TemporaryMidiFile
{
public:
  explicit TemporaryMidiFile(const std::vector<std::uint8_t>& bytes);
  ~TemporaryMidiFile();

  TemporaryMidiFile(const TemporaryMidiFile&) = delete;
  TemporaryMidiFile& operator=(const TemporaryMidiFile&) = delete;

  TemporaryMidiFile(TemporaryMidiFile&& other) noexcept;
  TemporaryMidiFile& operator=(TemporaryMidiFile&& other) noexcept;

  [[nodiscard]] const std::filesystem::path& path() const;

private:
  std::filesystem::path m_path;
};

TemporaryMidiFile tempoChangeMidi();
TemporaryMidiFile overlappingNotesMidi();

} // namespace midi_fixtures
