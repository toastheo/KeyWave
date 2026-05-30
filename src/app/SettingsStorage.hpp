#pragma once

#include <filesystem>
#include <optional>

#include "app/AppSettings.hpp"
#include "diagnostics/Diagnostics.hpp"

class SettingsStorage
{
public:
  SettingsStorage();
  explicit SettingsStorage(std::filesystem::path path);

  static std::filesystem::path defaultSettingsPath();

  [[nodiscard]] const std::filesystem::path& path() const;

  [[nodiscard]] std::optional<AppSettings> load(
    DiagnosticSink& diagnostics = nullDiagnosticSink()) const;
  [[nodiscard]] static std::optional<AppSettings> load(
    const std::filesystem::path& path, DiagnosticSink& diagnostics = nullDiagnosticSink());

  [[nodiscard]] bool save(const AppSettings& settings,
                          DiagnosticSink& diagnostics = nullDiagnosticSink()) const;
  [[nodiscard]] static bool save(const AppSettings& settings,
                                 const std::filesystem::path& path,
                                 DiagnosticSink& diagnostics = nullDiagnosticSink());

private:
  std::filesystem::path m_path;
};
