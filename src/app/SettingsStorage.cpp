#include "app/SettingsStorage.hpp"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <utility>

#include "app/AppSettingsSerializer.hpp"

namespace {

std::filesystem::path fallbackSettingsPath()
{
  return std::filesystem::current_path() / "keywave-settings.json";
}

} // namespace

SettingsStorage::SettingsStorage()
    : m_path(defaultSettingsPath())
{}

SettingsStorage::SettingsStorage(std::filesystem::path path)
    : m_path(std::move(path))
{}

std::filesystem::path SettingsStorage::defaultSettingsPath()
{
#if defined(_WIN32)
  const char* appData = std::getenv("APPDATA");
  if (appData != nullptr && appData[0] != '\0') {
    return std::filesystem::path(appData) / "KeyWave" / "settings.json";
  }
#elif defined(__APPLE__)
  const char* home = std::getenv("HOME");
  if (home != nullptr && home[0] != '\0') {
    return std::filesystem::path(home) / "Library" / "Application Support" / "KeyWave" /
           "settings.json";
  }
#else
  const char* xdgConfigHome = std::getenv("XDG_CONFIG_HOME");
  if (xdgConfigHome != nullptr && xdgConfigHome[0] != '\0') {
    return std::filesystem::path(xdgConfigHome) / "keywave" / "settings.json";
  }

  const char* home = std::getenv("HOME");
  if (home != nullptr && home[0] != '\0') {
    return std::filesystem::path(home) / ".config" / "keywave" / "settings.json";
  }
#endif

  return fallbackSettingsPath();
}

const std::filesystem::path& SettingsStorage::path() const
{
  return m_path;
}

std::optional<AppSettings> SettingsStorage::load() const
{
  return load(m_path);
}

std::optional<AppSettings> SettingsStorage::load(const std::filesystem::path& path)
{
  try {
    if (!std::filesystem::exists(path)) {
      std::cout << "Settings file not found, using defaults: " << path << '\n';
      return std::nullopt;
    }

    std::ifstream input(path);
    if (!input) {
      std::cerr << "Warning: could not open settings file, using defaults: " << path << '\n';
      return std::nullopt;
    }

    const auto json = nlohmann::json::parse(input);
    return AppSettingsSerializer::deserialize(json, AppSettings{});
  }
  catch (const nlohmann::json::exception& exception) {
    std::cerr << "Warning: malformed settings file, using defaults: " << path << " ("
              << exception.what() << ")\n";
  }
  catch (const std::exception& exception) {
    std::cerr << "Warning: could not load settings file, using defaults: " << path << " ("
              << exception.what() << ")\n";
  }

  return std::nullopt;
}

bool SettingsStorage::save(const AppSettings& settings) const
{
  return save(settings, m_path);
}

bool SettingsStorage::save(const AppSettings& settings, const std::filesystem::path& path)
{
  try {
    const auto parentPath = path.parent_path();
    if (!parentPath.empty()) {
      std::filesystem::create_directories(parentPath);
    }

    auto tempPath = path;
    tempPath += ".tmp";

    {
      std::ofstream output(tempPath, std::ios::trunc);
      if (!output) {
        std::cerr << "Warning: could not open temporary settings file for writing: " << tempPath
                  << '\n';
        return false;
      }

      output << AppSettingsSerializer::serialize(sanitizeAppSettings(settings)).dump(2) << '\n';
      if (!output) {
        std::cerr << "Warning: could not write settings file: " << tempPath << '\n';
        return false;
      }
    }

    if (std::filesystem::exists(path)) {
      std::filesystem::remove(path);
    }
    std::filesystem::rename(tempPath, path);
    return true;
  }
  catch (const std::exception& exception) {
    std::cerr << "Warning: could not save settings file: " << path << " (" << exception.what()
              << ")\n";
  }

  return false;
}
