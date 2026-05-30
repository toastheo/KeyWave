#pragma once

#include <filesystem>
#include <optional>

#include "diagnostics/Diagnostics.hpp"
#include "midi/MidiTimeline.hpp"

class MidiFileLoader
{
public:
  static std::optional<MidiTimeline> loadFromFile(
    const std::filesystem::path& path, DiagnosticSink& diagnostics = nullDiagnosticSink());
};
