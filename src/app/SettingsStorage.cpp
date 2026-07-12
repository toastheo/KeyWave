#include "app/SettingsStorage.hpp"

#include <cstdlib>
#include <exception>
#include <filesystem>
#include <fstream>
#include <ios>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <system_error>
#include <utility>

#include "app/AppSettings.hpp"
#include "app/AppSettingsSerializer.hpp"
#include "diagnostics/Diagnostics.hpp"

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

std::optional<AppSettings> SettingsStorage::load(DiagnosticSink& diagnostics) const
{
  return load(m_path, diagnostics);
}

std::optional<AppSettings> SettingsStorage::load(const std::filesystem::path& path,
                                                 DiagnosticSink& diagnostics)
{
  try {
    if (!std::filesystem::exists(path)) {
      reportInfo(diagnostics, "Settings file not found, using defaults: " + path.string());
      return std::nullopt;
    }

    const std::ifstream input(path);
    if (!input) {
      reportWarning(diagnostics,
                    "Warning: could not open settings file, using defaults: " + path.string());
      return std::nullopt;
    }

    std::ostringstream contents;
    contents << input.rdbuf();
    return AppSettingsSerializer::deserialize(contents.str(), AppSettings{});
  }
  catch (const std::invalid_argument& exception) {
    std::ostringstream message;
    message << "Warning: malformed settings file, using defaults: " << path << " ("
            << exception.what() << ")";
    reportWarning(diagnostics, message.str());
  }
  catch (const std::exception& exception) {
    std::ostringstream message;
    message << "Warning: could not load settings file, using defaults: " << path << " ("
            << exception.what() << ")";
    reportWarning(diagnostics, message.str());
  }

  return std::nullopt;
}

bool SettingsStorage::save(const AppSettings& settings, DiagnosticSink& diagnostics) const
{
  return save(settings, m_path, diagnostics);
}

bool SettingsStorage::save(const AppSettings& settings,
                           const std::filesystem::path& path,
                           DiagnosticSink& diagnostics)
{
  try {
    const auto parentPath = path.parent_path();
    if (!parentPath.empty()) {
      std::filesystem::create_directories(parentPath);
    }

    // Create and write through a temporary file, so if it fails, we don't lose the valid settings.
    auto tempPath = path;
    tempPath += ".tmp";

    {
      std::ofstream output(tempPath, std::ios::trunc);
      if (!output) {
        reportWarning(diagnostics,
                      "Warning: could not open temporary settings file for writing: " +
                        tempPath.string());
        return false;
      }

      output << AppSettingsSerializer::serialize(sanitizeAppSettings(settings), 2) << '\n';
      if (!output) {
        reportWarning(diagnostics, "Warning: could not write settings file: " + tempPath.string());
        return false;
      }
    }

    std::error_code renameError;
    std::filesystem::rename(tempPath, path, renameError);
    if (renameError) {
      std::ostringstream message;
      message << "Warning: could not save settings file: " << path << " (" << renameError.message()
              << ")";
      reportWarning(diagnostics, message.str());
      return false;
    }

    return true;
  }
  catch (const std::exception& exception) {
    std::ostringstream message;
    message << "Warning: could not save settings file: " << path << " (" << exception.what() << ")";
    reportWarning(diagnostics, message.str());
  }

  return false;
}
