#pragma once

#include <memory>
#include <string>
#include <vector>

#include "diagnostics/Diagnostics.hpp"
#include "input/Key.hpp"
#include "render/RenderTypes.hpp"

struct GLFWwindow;

enum class PlatformWindowDisplayMode : std::uint8_t
{
  Windowed,
  BorderlessFullscreen,
  ExclusiveFullscreen,
};

[[nodiscard]] constexpr bool shouldRestoreNativeWindowBeforeChangingDisplayMode(
  const PlatformWindowDisplayMode currentMode, const bool nativeWindowMaximized)
{
  return currentMode == PlatformWindowDisplayMode::Windowed && nativeWindowMaximized;
}

struct WindowConfig
{
  std::string title;
  int width = 1280;
  int height = 720;
  bool vsyncEnabled = true;
};

struct WindowedSize
{
  int width = 1280;
  int height = 720;
};

using NativeProcAddress = void (*)();
using NativeProcAddressLoader = NativeProcAddress (*)(const char* name);

struct GlfwWindowDeleter
{
  void operator()(GLFWwindow* window) const;
};

class Window
{
public:
  Window() = default;
  ~Window();

  Window(const Window&) = delete;
  Window& operator=(const Window&) = delete;

  bool initialize(const WindowConfig& config, DiagnosticSink& diagnostics = nullDiagnosticSink());
  void shutdown();

  [[nodiscard]] bool shouldClose() const;

  static void pollEvents();
  [[nodiscard]] std::vector<Key> consumePressedKeys();
  void swapBuffers() const;
  [[nodiscard]] FramebufferSize framebufferSize() const;
  [[nodiscard]] GLFWwindow* nativeHandle() const;
  [[nodiscard]] static NativeProcAddressLoader nativeProcAddressLoader();
  [[nodiscard]] bool setDisplayMode(PlatformWindowDisplayMode mode,
                                    WindowedSize windowedSize,
                                    DiagnosticSink& diagnostics = nullDiagnosticSink());
  void setWindowedSize(WindowedSize size);
  void setVsyncEnabled(bool enabled) const;

private:
  std::unique_ptr<GLFWwindow, GlfwWindowDeleter> m_handle;
  std::vector<Key> m_pressedKeys;
  bool m_ownsGlfw = false;
  PlatformWindowDisplayMode m_displayMode = PlatformWindowDisplayMode::Windowed;
  int m_windowedX = 100;
  int m_windowedY = 100;
  int m_windowedWidth = 1280;
  int m_windowedHeight = 720;
};
