#pragma once

#include <atomic>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <span>
#include <string>
#include <vector>

// Manages a child process.
//
// This class is designed for FFmpeg-based encoding rather than as a general-
// purpose process abstraction. In particular, stdout and stderr are merged
// into a bounded diagnostic buffer, as FFmpeg write its encoded output
// directly to a file.
//
// The process is started without a shell, using UTF-8 arguments.
// Its lifetime and resources are managed automatically.
//
// Blocking operations support cancellation and timeouts.
// Run process operations on a worker thread.
class ChildProcess final
{
public:
  ChildProcess(const std::filesystem::path& executable, const std::vector<std::string>& arguments);
  ~ChildProcess();
  ChildProcess(const ChildProcess&) = delete;
  ChildProcess& operator=(const ChildProcess&) = delete;

  void writeInput(std::span<const std::uint8_t> bytes,
                  const std::atomic_bool& cancelled,
                  std::chrono::milliseconds stallTimeout = std::chrono::seconds(30));
  void closeInput();
  int wait(const std::atomic_bool& cancelled,
           std::chrono::milliseconds timeout = std::chrono::seconds(30));
  [[nodiscard]] std::string diagnostics() const;
  [[nodiscard]] static std::filesystem::path findExecutable(const std::string& name);
  static constexpr std::size_t diagnosticLimit = 65536; // 64 KiB

private:
  struct Impl;
  std::unique_ptr<Impl> m_impl;
};
