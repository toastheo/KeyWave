#pragma once

#include <filesystem>
#include <optional>

struct AppConfig
{
  std::optional<std::filesystem::path> midiFilePath;
};

[[nodiscard]] std::optional<AppConfig> parseAppConfig(int argc, char* const* argv);
