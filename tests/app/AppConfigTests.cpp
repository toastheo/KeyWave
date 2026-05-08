#include "app/AppConfig.hpp"

#include <catch2/catch_test_macros.hpp>

#include <array>
#include <filesystem>
#include <string>

namespace {

TEST_CASE("parseAppConfig accepts no MIDI file argument", "[app][config]")
{
  std::string executableName = "KeyWave";
  const std::array argv{executableName.data()};

  const auto config = parseAppConfig(argv.size(), argv.data());

  REQUIRE(config.has_value());
  CHECK_FALSE(config->midiFilePath.has_value());
}

TEST_CASE("parseAppConfig stores a single MIDI file argument", "[app][config]")
{
  std::string executableName = "KeyWave";
  std::string midiPath = "songs/example.mid";
  const std::array argv{executableName.data(), midiPath.data()};

  const auto config = parseAppConfig(argv.size(), argv.data());

  REQUIRE(config.has_value());
  REQUIRE(config->midiFilePath.has_value());
  CHECK(*config->midiFilePath == std::filesystem::path{"songs/example.mid"});
}

TEST_CASE("parseAppConfig rejects extra arguments", "[app][config]")
{
  std::string executableName = "KeyWave";
  std::string firstPath = "first.mid";
  std::string secondPath = "second.mid";
  const std::array argv{executableName.data(), firstPath.data(), secondPath.data()};

  const auto config = parseAppConfig(argv.size(), argv.data());

  CHECK_FALSE(config.has_value());
}

} // namespace
