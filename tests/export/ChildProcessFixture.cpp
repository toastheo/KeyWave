#include <chrono>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <string>
#include <thread>

// include-cleaner attributes Win32 declarations to internal SDK headers.
// Keep the supported windows.h umbrella; exempt only Win32 sections.
#ifdef _WIN32
// NOLINTBEGIN(misc-include-cleaner)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
// NOLINTEND(misc-include-cleaner)
#endif

int main(int argc, char** argv)
try {
  const auto executable = std::filesystem::path(argv[0]).stem().string();
  if (argc > 1 && std::string(argv[1]) == "echo") {
    for (int i = 2; i < argc; ++i) {
      const std::string value(argv[i]);
      if (std::printf("%zu:%s\n", value.size(), value.c_str()) < 0) {
        return 1;
      }
    }
    return 0;
  }
  if (argc > 1 && std::string(argv[1]) == "flood") {
    for (int i = 0; i < 100000; ++i) {
      if (std::fputc('x', stderr) == EOF) {
        return 1;
      }
    }
    if (std::fputs("diagnostic-end", stderr) < 0) {
      return 1;
    }
    return 7;
  }
  if (argc > 1 && std::string(argv[1]) == "console") {
#ifdef _WIN32
    // NOLINTBEGIN(misc-include-cleaner)
    return GetConsoleWindow() == nullptr ? 0 : 2;
// NOLINTEND(misc-include-cleaner)
#else
    return 0;
#endif
  }
  if (argc > 1 && std::string(argv[1]) == "block") {
    std::this_thread::sleep_for(std::chrono::minutes(10));
    return 0;
  }
  if (argc > 2 && std::string(argv[2]) == "-h") {
    if (std::fputs(executable.find("no-codec") == std::string::npos ? "Encoder libx264 [fixture]\n"
                                                                    : "Unknown encoder\n",
                   stdout) < 0) {
      return 1;
    }
    return 0;
  }
  if (executable.find("crash") != std::string::npos) {
    for (int i = 0; i < 100000; ++i) {
      if (std::fputc('x', stderr) == EOF) {
        return 1;
      }
    }
    if (std::fputs("encoder-crash", stderr) < 0) {
      return 1;
    }
    return 7;
  }
  if (executable.find("block") != std::string::npos) {
    std::this_thread::sleep_for(std::chrono::minutes(10));
    return 0;
  }
  // The fixture's successful variant consumes frames then creates a tiny file.
  // Actual video validity is exercised by the separate real-FFmpeg tests.
  while (std::fgetc(stdin) != EOF) {
  }
  if (argc > 1) {
    std::ofstream output(
      std::filesystem::path(std::u8string(reinterpret_cast<const char8_t*>(argv[argc - 1]))));
    output << "video";
  }
  return 0;
}
catch (...) {
  return 1;
}
