#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <string>

#include "app/SettingsStorage.hpp"

namespace {

std::filesystem::path uniqueSettingsPath()
{
  const auto suffix = std::chrono::steady_clock::now().time_since_epoch().count();
  return std::filesystem::temp_directory_path() /
         ("keywave-settings-test-" + std::to_string(suffix)) / "nested" / "settings.json";
}

TEST_CASE("SettingsStorage returns nullopt when the settings file is missing", "[app][settings]")
{
  SettingsStorage const storage;
  const auto path = uniqueSettingsPath();

  CHECK_FALSE(storage.load(path).has_value());
}

TEST_CASE("SettingsStorage saves settings and creates parent directories", "[app][settings]")
{
  SettingsStorage const storage;
  const auto path = uniqueSettingsPath();

  AppSettings settings;
  settings.renderer.clearColor = Color{.r = 0.7f, .g = 0.6f, .b = 0.5f, .a = 1.0f};
  settings.window.displayMode = WindowDisplayMode::BorderlessFullscreen;
  settings.window.width = 1920;
  settings.window.height = 1080;
  settings.window.vsyncEnabled = false;
  settings.window.fpsLimit = 144;

  CHECK(storage.save(settings, path));
  CHECK(std::filesystem::exists(path));

  const auto loaded = SettingsStorage::load(path);
  REQUIRE(loaded.has_value());
  CHECK(loaded->renderer.clearColor.r == Catch::Approx(0.7f));
  CHECK(loaded->renderer.clearColor.g == Catch::Approx(0.6f));
  CHECK(loaded->renderer.clearColor.b == Catch::Approx(0.5f));
  CHECK(loaded->window.displayMode == WindowDisplayMode::BorderlessFullscreen);
  CHECK(loaded->window.width == 1920);
  CHECK(loaded->window.height == 1080);
  CHECK_FALSE(loaded->window.vsyncEnabled);
  CHECK(loaded->window.fpsLimit == 144);
}

TEST_CASE("SettingsStorage saves over existing settings", "[app][settings]")
{
  SettingsStorage const storage;
  const auto path = uniqueSettingsPath();

  AppSettings originalSettings;
  originalSettings.renderer.clearColor = Color{.r = 0.1f, .g = 0.2f, .b = 0.3f, .a = 1.0f};
  REQUIRE(storage.save(originalSettings, path));

  AppSettings replacementSettings;
  replacementSettings.renderer.clearColor = Color{.r = 0.8f, .g = 0.7f, .b = 0.6f, .a = 1.0f};
  REQUIRE(storage.save(replacementSettings, path));

  const auto loaded = SettingsStorage::load(path);
  REQUIRE(loaded.has_value());
  CHECK(loaded->renderer.clearColor.r == Catch::Approx(0.8f));
  CHECK(loaded->renderer.clearColor.g == Catch::Approx(0.7f));
  CHECK(loaded->renderer.clearColor.b == Catch::Approx(0.6f));
}

TEST_CASE("SettingsStorage returns nullopt for malformed JSON", "[app][settings]")
{
  SettingsStorage const storage;
  const auto path = uniqueSettingsPath();
  std::filesystem::create_directories(path.parent_path());
  {
    std::ofstream out(path);
    out << "{ invalid json";
  }

  CHECK_FALSE(storage.load(path).has_value());
}

} // namespace
