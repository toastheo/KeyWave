#pragma once

#include <filesystem>
#include <optional>

#include "diagnostics/Diagnostics.hpp"

struct VideoSaveSelection
{
  std::filesystem::path path;
  bool allowOverwrite = false;
};

class VideoSaveDialog
{
public:
  static std::optional<VideoSaveSelection> save(DiagnosticSink& diagnostics);
  // Only use after a successful native save dialog (which confirms replacements).
  static VideoSaveSelection selectionForConfirmedPath(const std::filesystem::path& path);
};
