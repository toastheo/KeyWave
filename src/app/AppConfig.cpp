#include "app/AppConfig.hpp"

#include <iostream>

namespace {

void printUsage()
{
  std::cerr << "Usage:\n";
  std::cerr << "  KeyWave [path-to-midi-file]\n\n";
  std::cerr << "Examples:\n";
  std::cerr << "  KeyWave assets/test-midi/test.mid\n";
  std::cerr << "  KeyWave C:\\Music\\song.mid\n";
}

} // namespace

std::optional<AppConfig> parseAppConfig(const int argc, char* const* argv)
{
  if (argc <= 1) {
    return AppConfig{};
  }

  if (argc == 2) {
    return AppConfig{
      .midiFilePath = std::filesystem::path{argv[1]},
    };
  }

  printUsage();
  return std::nullopt;
}
