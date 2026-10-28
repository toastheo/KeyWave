#pragma once

#include <filesystem>
#include <optional>

#include "diagnostics/Diagnostics.hpp"

class VideoSaveDialog
{
public:
  static std::optional<std::filesystem::path> save(DiagnosticSink& diagnostics);
};
