#include "app/AppConfig.hpp"
#include "app/Application.hpp"

#include <iostream>

int main(const int argc, char** argv)
{
  const auto config = parseAppConfig(argc, argv);
  if (!config.has_value()) {
    return 1;
  }

  Application application{*config};

  if (!application.initialize()) {
    std::cerr << "KeyWave failed to initialize.\n";
    return 1;
  }

  application.run();
  return 0;
}
