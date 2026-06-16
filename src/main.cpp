/*
 * ┌───────────────────────────────────────────────┐
 * │                    __    __                   │
 * │     /\ /\___ _   _/ / /\ \ \__ ___   _____    │
 * │    / //_/ _ \ | | \ \/  \/ / _` \ \ / / _ \   │
 * │   / __ \  __/ |_| |\  /\  / (_| |\ V /  __/   │
 * │   \/  \/\___|\__, | \/  \/ \__,_| \_/ \___|   │
 * │              |___/                            │
 * │                                               │
 * │         ________________________              │
 * │       /           ___ ___       '.            │
 * │       |\ .-------|:::/:::|--------;           │
 * │       | |        |:::|:::|        |           │
 * │       |\|_  ___ _|:::/:::| __ ____|           │
 * │       |    __  ___""" """_ ____ __`\          │
 * │       | |\##\\###\\##\\###\\##\\### \         │
 * │       \ \ \\\\\\\\\\\\\\\\\\\\\\\\\\ \        │
 * │       |\ \||||||||||||||||||||||||||_|        │
 * │       | | ;"""""""""""""""""""""""";"|        │
 * │       | | |"""""".----------.""""""| |        │
 * │       | | |     |\           \  |  | |        │
 * │       | | |-----|\\___________\-|  | |        │
 * │       | | |     | |---------- |  \ | |        │
 * │       `\| |     | |         | |   `| |        │
 * │         \_|       |           |    `\|        │
 * │                                               │
 * │ KeyWave                                       │
 * │ A MIDI visualizer by Theodor Weinreich        │
 * │                                               │
 * │ GitHub:  https://github.com/toastheo/KeyWave  │
 * │ License: Licensed under GPL-3.0 license       │
 * │ ASCII Art by Joan Stark:                      │
 * │ https://asciiart.website/art/5478             │
 * └───────────────────────────────────────────────┘
 */

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
