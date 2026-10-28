#include <chrono>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <string>
#include <thread>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

int main(int argc, char** argv)
{
  const auto executable = std::filesystem::path(argv[0]).stem().string();
  if (argc > 1 && std::string(argv[1]) == "echo") {
    for (int i = 2; i < argc; ++i) {
      const std::string value(argv[i]);
      std::printf("%zu:%s\n", value.size(), value.c_str());
    }
    return 0;
  }
  if (argc > 1 && std::string(argv[1]) == "flood") {
    for (int i = 0; i < 100000; ++i) { std::fputc('x', stderr); }
    std::fputs("diagnostic-end", stderr);
    return 7;
  }
  if (argc > 1 && std::string(argv[1]) == "console") {
#ifdef _WIN32
    return GetConsoleWindow() == nullptr ? 0 : 2;
#else
    return 0;
#endif
  }
  if (argc > 1 && std::string(argv[1]) == "block") {
    std::this_thread::sleep_for(std::chrono::minutes(10));
    return 0;
  }
  if (argc > 2 && std::string(argv[2]) == "-h") {
    std::fputs(executable.find("no-codec") == std::string::npos ?
                 "Encoder libx264 [fixture]\n" : "Unknown encoder\n", stdout);
    return 0;
  }
  if (executable.find("crash") != std::string::npos) {
    for (int i = 0; i < 100000; ++i) { std::fputc('x', stderr); }
    std::fputs("encoder-crash", stderr);
    return 7;
  }
  if (executable.find("block") != std::string::npos) {
    std::this_thread::sleep_for(std::chrono::minutes(10));
    return 0;
  }
  // The fixture's successful variant consumes frames then creates a tiny file.
  // Actual video validity is exercised by the separate real-FFmpeg tests.
  while (std::fgetc(stdin) != EOF) {}
  if (argc > 1) { std::ofstream output(std::filesystem::u8path(argv[argc - 1])); output << "video"; }
  return 0;
}
