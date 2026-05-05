#pragma once

#include "midi/MidiTimeline.hpp"

#include <filesystem>
#include <optional>

class MidiFileLoader {
public:
  static std::optional<MidiTimeline> loadFromFile(const std::filesystem::path& path);
};
