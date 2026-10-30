#include "platform/ChildProcess.hpp"

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <memory>
#include <mutex>
#include <span>
#include <stdexcept>
#include <string>
#include <system_error>
#include <thread>
#include <vector>

// include-cleaner attributes Win32 declarations to internal SDK headers.
// Keep the supported windows.h umbrella; exempt only Win32 sections.
#ifdef _WIN32
// NOLINTBEGIN(misc-include-cleaner)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
// NOLINTEND(misc-include-cleaner)
#else
#include <cerrno>
#include <csignal>
#include <fcntl.h>
#include <poll.h>
#include <spawn.h>
#include <sys/wait.h>
#include <unistd.h>
extern char** environ;
#endif

namespace {
#ifdef _WIN32
// NOLINTBEGIN(misc-include-cleaner)
struct Handle
{
  HANDLE value = INVALID_HANDLE_VALUE;
  ~Handle()
  {
    reset();
  }
  void reset(HANDLE next = INVALID_HANDLE_VALUE)
  {
    if (value != INVALID_HANDLE_VALUE && value != nullptr) {
      CloseHandle(value);
    }
    value = next;
  }
};

std::wstring wide(const std::string& utf8)
{
  if (utf8.empty()) {
    return {};
  }
  const int size = MultiByteToWideChar(
    CP_UTF8, MB_ERR_INVALID_CHARS, utf8.data(), static_cast<int>(utf8.size()), nullptr, 0);
  if (size == 0) {
    throw std::invalid_argument("Invalid UTF-8 process argument");
  }
  std::wstring result(size, L'\0');
  MultiByteToWideChar(
    CP_UTF8, MB_ERR_INVALID_CHARS, utf8.data(), static_cast<int>(utf8.size()), result.data(), size);
  return result;
}

std::wstring quote(const std::wstring& argument)
{
  std::wstring result = L"\"";
  std::size_t slashes = 0;
  for (const auto c : argument) {
    if (c == L'\\') {
      ++slashes;
      continue;
    }
    result.append(c == L'"' ? slashes * 2 + 1 : slashes, L'\\');
    result += c;
    slashes = 0;
  }
  result.append(slashes * 2, L'\\');
  return result + L'"';
}

void winError(const char* context)
{
  throw std::system_error(static_cast<int>(GetLastError()), std::system_category(), context);
}
// NOLINTEND(misc-include-cleaner)
#else
struct Descriptor
{
  int value = -1;
  ~Descriptor()
  {
    reset();
  }
  void reset(int next = -1)
  {
    if (value >= 0) {
      close(value);
    }
    value = next;
  }
};
#endif
} // namespace

struct ChildProcess::Impl
{
  mutable std::mutex mutex;
  std::string output;
  std::thread reader;
#ifdef _WIN32
  // NOLINTBEGIN(misc-include-cleaner)
  Handle process, input, errors;
// NOLINTEND(misc-include-cleaner)
#else
  pid_t pid = -1;
  Descriptor input, errors;
#endif
  bool reaped = false;
  int exitCode = -1;

  void append(const char* bytes, std::size_t count)
  {
    std::lock_guard const lock(mutex);
    output.append(bytes, count);
    if (output.size() > diagnosticLimit) {
      output.erase(0, output.size() - diagnosticLimit);
    }
  }

  void drain()
  {
    std::array<char, 4096> buffer{};
    for (;;) {
#ifdef _WIN32
      // NOLINTBEGIN(misc-include-cleaner)
      DWORD count = 0;
      if (!ReadFile(
            errors.value, buffer.data(), static_cast<DWORD>(buffer.size()), &count, nullptr) ||
          count == 0) {
        break;
      }
// NOLINTEND(misc-include-cleaner)
#else
      const auto count = read(errors.value, buffer.data(), buffer.size());
      if (count < 0 && errno == EINTR) {
        continue;
      }
      if (count <= 0) {
        break;
      }
#endif
      append(buffer.data(), static_cast<std::size_t>(count));
    }
  }

  void terminate()
  {
#ifdef _WIN32
    // NOLINTBEGIN(misc-include-cleaner)
    if (!reaped && process.value != INVALID_HANDLE_VALUE) {
      TerminateProcess(process.value, 1);
      WaitForSingleObject(process.value, INFINITE);
      reaped = true;
    }
// NOLINTEND(misc-include-cleaner)
#else
    if (!reaped && pid > 0) {
      kill(pid, SIGKILL);
      int status = 0;
      while (waitpid(pid, &status, 0) < 0 && errno == EINTR) {
      }
      reaped = true;
    }
#endif
  }
  ~Impl()
  {
    terminate();
    input.reset();
    if (reader.joinable()) {
      reader.join();
    }
  }
};

ChildProcess::ChildProcess(const std::filesystem::path& executable,
                           const std::vector<std::string>& arguments)
    : m_impl(std::make_unique<Impl>())
{
  if (executable.empty()) {
    throw std::invalid_argument("Empty process executable");
  }
#ifdef _WIN32
  // NOLINTBEGIN(misc-include-cleaner)
  // Overlapped server writes allow cancellation without a blocked UI or writer.
  static std::atomic_uint64_t serial{0};
  const auto pipeName = L"\\\\.\\pipe\\KeyWave-" + std::to_wstring(GetCurrentProcessId()) + L"-" +
                        std::to_wstring(++serial);
  m_impl->input.value =
    CreateNamedPipeW(pipeName.c_str(),
                     PIPE_ACCESS_OUTBOUND | FILE_FLAG_OVERLAPPED | FILE_FLAG_FIRST_PIPE_INSTANCE,
                     PIPE_TYPE_BYTE | PIPE_WAIT,
                     1,
                     65536,
                     65536,
                     0,
                     nullptr);
  if (m_impl->input.value == INVALID_HANDLE_VALUE) {
    winError("Creating encoder input pipe");
  }
  SECURITY_ATTRIBUTES security{sizeof(SECURITY_ATTRIBUTES), nullptr, TRUE};
  Handle childInput;
  childInput.value = CreateFileW(
    pipeName.c_str(), GENERIC_READ, 0, &security, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
  if (childInput.value == INVALID_HANDLE_VALUE) {
    winError("Opening encoder input pipe");
  }
  Handle childErrors;
  if (!CreatePipe(&m_impl->errors.value, &childErrors.value, &security, 0)) {
    winError("Creating encoder diagnostic pipe");
  }
  if (!SetHandleInformation(m_impl->errors.value, HANDLE_FLAG_INHERIT, 0)) {
    winError("Configuring encoder diagnostic pipe");
  }
  // Inherit only the three standard handles, never unrelated app resources.
  SIZE_T attributeSize = 0;
  InitializeProcThreadAttributeList(nullptr, 1, 0, &attributeSize);
  std::vector<std::uint8_t> attributes(attributeSize);
  auto* list = reinterpret_cast<LPPROC_THREAD_ATTRIBUTE_LIST>(attributes.data());
  if (!InitializeProcThreadAttributeList(list, 1, 0, &attributeSize)) {
    winError("Initializing process attributes");
  }
  struct AttributeGuard
  {
    LPPROC_THREAD_ATTRIBUTE_LIST list;
    ~AttributeGuard()
    {
      DeleteProcThreadAttributeList(list);
    }
  } const guard{list};
  std::array inherited{childInput.value, childErrors.value};
  if (!UpdateProcThreadAttribute(list,
                                 0,
                                 PROC_THREAD_ATTRIBUTE_HANDLE_LIST,
                                 static_cast<void*>(inherited.data()),
                                 sizeof(inherited),
                                 nullptr,
                                 nullptr)) {
    winError("Configuring process handle inheritance");
  }
  STARTUPINFOEXW startup{};
  startup.StartupInfo.cb = sizeof(startup);
  startup.StartupInfo.dwFlags = STARTF_USESTDHANDLES;
  startup.StartupInfo.hStdInput = childInput.value;
  startup.StartupInfo.hStdOutput = childErrors.value;
  startup.StartupInfo.hStdError = childErrors.value;
  startup.lpAttributeList = list;
  auto command = quote(executable.wstring());
  for (const auto& argument : arguments) {
    command += L" " + quote(wide(argument));
  }
  PROCESS_INFORMATION info{};
  if (!CreateProcessW(executable.c_str(),
                      command.data(),
                      nullptr,
                      nullptr,
                      TRUE,
                      CREATE_NO_WINDOW | EXTENDED_STARTUPINFO_PRESENT,
                      nullptr,
                      nullptr,
                      &startup.StartupInfo,
                      &info)) {
    winError("Starting FFmpeg process");
  }
  m_impl->process.value = info.hProcess;
  CloseHandle(info.hThread);
  childInput.reset();
  childErrors.reset();
// NOLINTEND(misc-include-cleaner)
#else
  int inputPipe[2], outputPipe[2];
  if (pipe(inputPipe) != 0) {
    throw std::system_error(errno, std::generic_category(), "pipe");
  }
  Descriptor childInput;
  childInput.value = inputPipe[0];
  m_impl->input.value = inputPipe[1];
  if (pipe(outputPipe) != 0) {
    throw std::system_error(errno, std::generic_category(), "pipe");
  }
  m_impl->errors.value = outputPipe[0];
  Descriptor childOutput;
  childOutput.value = outputPipe[1];
  for (int fd : {inputPipe[0], inputPipe[1], outputPipe[0], outputPipe[1]}) {
    if (fcntl(fd, F_SETFD, FD_CLOEXEC) < 0) {
      throw std::runtime_error("Setting pipe CLOEXEC");
    }
  }
  posix_spawn_file_actions_t actions;
  posix_spawn_file_actions_init(&actions);
  struct ActionsGuard
  {
    posix_spawn_file_actions_t* actions;
    ~ActionsGuard()
    {
      posix_spawn_file_actions_destroy(actions);
    }
  } guard{&actions};
  posix_spawn_file_actions_adddup2(&actions, childInput.value, STDIN_FILENO);
  posix_spawn_file_actions_adddup2(&actions, childOutput.value, STDOUT_FILENO);
  posix_spawn_file_actions_adddup2(&actions, childOutput.value, STDERR_FILENO);
  std::vector<std::string> storage{executable.string()};
  storage.insert(storage.end(), arguments.begin(), arguments.end());
  std::vector<char*> argv;
  for (auto& arg : storage) {
    argv.push_back(arg.data());
  }
  argv.push_back(nullptr);
  const auto error =
    posix_spawn(&m_impl->pid, executable.c_str(), &actions, nullptr, argv.data(), environ);
  if (error != 0) {
    throw std::system_error(error, std::generic_category(), "Starting FFmpeg");
  }
  childInput.reset();
  childOutput.reset();
  if (fcntl(m_impl->input.value, F_SETFL, O_NONBLOCK) < 0) {
    throw std::runtime_error("Setting encoder input nonblocking");
  }
#endif
  m_impl->reader = std::thread([impl = m_impl.get()] { impl->drain(); });
}

ChildProcess::~ChildProcess() = default;

void ChildProcess::writeInput(std::span<const std::uint8_t> bytes,
                              const std::atomic_bool& cancelled,
                              std::chrono::milliseconds stallTimeout)
{
  auto deadline = std::chrono::steady_clock::now() + stallTimeout;
#ifndef _WIN32
  // Block SIGPIPE on this worker only: consume a signal generated by EPIPE.
  sigset_t mask, oldMask, pending;
  sigemptyset(&mask);
  sigaddset(&mask, SIGPIPE);
  pthread_sigmask(SIG_BLOCK, &mask, &oldMask);
  sigpending(&pending);
  const bool wasPending = sigismember(&pending, SIGPIPE);
  struct SignalGuard
  {
    sigset_t oldMask;
    ~SignalGuard()
    {
      pthread_sigmask(SIG_SETMASK, &oldMask, nullptr);
    }
  } signalGuard{oldMask};
#endif
  while (!bytes.empty()) {
    if (cancelled) {
      throw std::runtime_error("Export cancelled");
    }
    if (std::chrono::steady_clock::now() >= deadline) {
      throw std::runtime_error("FFmpeg input stalled (pipe write timeout)");
    }
#ifdef _WIN32
    // NOLINTBEGIN(misc-include-cleaner)
    Handle event;
    event.value = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    if (event.value == nullptr) {
      winError("Creating pipe write event");
    }
    OVERLAPPED operation{};
    operation.hEvent = event.value;
    DWORD count = 0;
    const auto size = static_cast<DWORD>(std::min<std::size_t>(bytes.size(), 65536));
    if (!WriteFile(m_impl->input.value, bytes.data(), size, &count, &operation)) {
      if (GetLastError() != ERROR_IO_PENDING) {
        winError("Writing FFmpeg input");
      }
      for (;;) {
        const auto result = WaitForSingleObject(event.value, 50);
        if (result == WAIT_OBJECT_0) {
          break;
        }
        if (cancelled || std::chrono::steady_clock::now() >= deadline || result == WAIT_FAILED) {
          CancelIoEx(m_impl->input.value, &operation);
          // Keep OVERLAPPED and the caller buffer alive until cancellation completes.
          GetOverlappedResult(m_impl->input.value, &operation, &count, TRUE);
          throw std::runtime_error(cancelled ? "Export cancelled" : "FFmpeg input stalled");
        }
      }
      if (!GetOverlappedResult(m_impl->input.value, &operation, &count, FALSE)) {
        winError("Completing FFmpeg pipe write");
      }
    }
// NOLINTEND(misc-include-cleaner)
#else
    pollfd descriptor{m_impl->input.value, POLLOUT, 0};
    const auto ready = poll(&descriptor, 1, 50);
    if (ready < 0 && errno == EINTR) {
      continue;
    }
    if (ready < 0) {
      throw std::system_error(errno, std::generic_category(), "poll");
    }
    if (ready == 0) {
      continue;
    }
    const auto count =
      write(m_impl->input.value, bytes.data(), std::min<std::size_t>(bytes.size(), 65536));
    if (count < 0 && (errno == EINTR || errno == EAGAIN)) {
      continue;
    }
    if (count < 0) {
      if (errno == EPIPE && !wasPending) {
        timespec zero{};
        sigtimedwait(&mask, nullptr, &zero);
      }
      throw std::runtime_error("FFmpeg input pipe closed");
    }
#endif
    if (count == 0) {
      throw std::runtime_error("FFmpeg pipe accepted no data");
    }
    bytes = bytes.subspan(static_cast<std::size_t>(count));
    deadline = std::chrono::steady_clock::now() + stallTimeout;
  }
}

void ChildProcess::closeInput()
{
  m_impl->input.reset();
}

int ChildProcess::wait(const std::atomic_bool& cancelled, std::chrono::milliseconds timeout)
{
  const auto deadline = std::chrono::steady_clock::now() + timeout;
  while (!m_impl->reaped) {
    if (cancelled || std::chrono::steady_clock::now() >= deadline) {
      m_impl->terminate();
      throw std::runtime_error(cancelled ? "Export cancelled" : "FFmpeg process timed out");
    }
#ifdef _WIN32
    // NOLINTBEGIN(misc-include-cleaner)
    const auto result = WaitForSingleObject(m_impl->process.value, 50);
    if (result == WAIT_FAILED) {
      winError("Waiting for FFmpeg");
    }
    if (result == WAIT_OBJECT_0) {
      DWORD code = 0;
      if (!GetExitCodeProcess(m_impl->process.value, &code)) {
        winError("Reading FFmpeg exit code");
      }
      m_impl->exitCode = static_cast<int>(code);
      m_impl->reaped = true;
    }
// NOLINTEND(misc-include-cleaner)
#else
    int status = 0;
    const auto result = waitpid(m_impl->pid, &status, WNOHANG);
    if (result < 0 && errno != EINTR) {
      throw std::runtime_error("Waiting for FFmpeg failed");
    }
    if (result == m_impl->pid) {
      m_impl->exitCode = WIFEXITED(status) ? WEXITSTATUS(status) : -1;
      m_impl->reaped = true;
    }
    else {
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
#endif
  }
  if (m_impl->reader.joinable()) {
    m_impl->reader.join();
  }
  return m_impl->exitCode;
}

std::string ChildProcess::diagnostics() const
{
  std::lock_guard const lock(m_impl->mutex);
  return m_impl->output;
}

std::filesystem::path ChildProcess::findExecutable(const std::string& name)
{
#ifdef _WIN32
  // NOLINTBEGIN(misc-include-cleaner)
  std::array<wchar_t, 32768> module{};
  const DWORD size = GetModuleFileNameW(nullptr, module.data(), static_cast<DWORD>(module.size()));
  if (size > 0 && size < module.size()) {
    const auto local = std::filesystem::path(std::wstring(module.data(), size)).parent_path() /
                       (wide(name) + L".exe");
    if (std::filesystem::is_regular_file(local)) {
      return local;
    }
  }
  // Supplying PATH explicitly avoids the implicit current-directory executable search.
  const DWORD sizeNeeded = GetEnvironmentVariableW(L"PATH", nullptr, 0);
  std::wstring search(sizeNeeded, L'\0');
  if (sizeNeeded == 0 || !GetEnvironmentVariableW(L"PATH", search.data(), sizeNeeded)) {
    throw std::runtime_error("FFmpeg was not found beside the application or on PATH");
  }
  search.resize(sizeNeeded - 1);
  std::array<wchar_t, 32768> found{};
  const auto count = SearchPathW(search.c_str(),
                                 wide(name).c_str(),
                                 L".exe",
                                 static_cast<DWORD>(found.size()),
                                 found.data(),
                                 nullptr);
  if (count > 0 && count < found.size()) {
    return {std::wstring(found.data(), count)};
  }
// NOLINTEND(misc-include-cleaner)
#else
  const auto* path = std::getenv("PATH");
  if (path != nullptr) {
    std::string paths(path);
    std::size_t start = 0;
    while (start <= paths.size()) {
      const auto end = paths.find(':', start);
      const auto directory = paths.substr(start, end - start);
      if (!directory.empty()) {
        const auto candidate = std::filesystem::path(directory) / name;
        if (access(candidate.c_str(), X_OK) == 0 && std::filesystem::is_regular_file(candidate)) {
          return std::filesystem::absolute(candidate);
        }
      }
      if (end == std::string::npos) {
        break;
      }
      start = end + 1;
    }
  }
#endif
  throw std::runtime_error("FFmpeg was not found beside the application or on PATH");
}
