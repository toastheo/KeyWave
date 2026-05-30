#pragma once

#include <filesystem>
#include <optional>

#include "diagnostics/Diagnostics.hpp"

struct AppConfig
{
  std::optional<std::filesystem::path> midiFilePath;
};

[[nodiscard]] std::optional<AppConfig> parseAppConfig(
  int argc, char* const* argv, DiagnosticSink& diagnostics = nullDiagnosticSink());
