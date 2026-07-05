#include "app/AppConfig.hpp"

#include <optional>

#include "diagnostics/Diagnostics.hpp"

namespace {

void reportUsage(DiagnosticSink& diagnostics)
{
  reportError(diagnostics,
              "Usage:\n"
              "  KeyWave [path-to-midi-file]\n\n"
              "Examples:\n"
              "  KeyWave assets/test-midi/test.mid\n"
              "  KeyWave C:\\Music\\song.mid");
}

} // namespace

std::optional<AppConfig> parseAppConfig(const int argc,
                                        char* const* argv,
                                        DiagnosticSink& diagnostics)
{
  if (argc <= 1) {
    return AppConfig{};
  }

  if (argc == 2) {
    return AppConfig{
      .midiFilePath = std::filesystem::path{argv[1]},
    };
  }

  reportUsage(diagnostics);
  return std::nullopt;
}
