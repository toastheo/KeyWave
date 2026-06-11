#pragma once

#include <filesystem>
#include <optional>

#include "diagnostics/Diagnostics.hpp"

class MidiFileDialog
{
public:
  [[nodiscard]] static std::optional<std::filesystem::path> open(
    DiagnosticSink& diagnostics = nullDiagnosticSink());
};
