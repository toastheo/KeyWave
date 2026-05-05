#include "app/Application.hpp"

#include <iostream>

int main() {
  Application application;

  if (!application.initialize()) {
    std::cerr << "KeyWave failed to initialize.\n";
    return 1;
  }

  application.run();
  return 0;
}
