#include "app/AppConfig.hpp"
#include "app/Application.hpp"
#include "diagnostics/Diagnostics.hpp"

int main(const int argc, char** argv)
{
  auto& diagnostics = consoleDiagnosticSink();
  const auto config = parseAppConfig(argc, argv, diagnostics);
  if (!config.has_value()) {
    return 1;
  }

  Application application{*config};

  if (!application.initialize()) {
    reportError(diagnostics, "KeyWave failed to initialize.");
    return 1;
  }

  application.run();
  return 0;
}
