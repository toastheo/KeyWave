#pragma once

#include <string>
#include <vector>

#include "diagnostics/Diagnostics.hpp"
#include "input/Key.hpp"
#include "render/RenderTypes.hpp"

enum class PlatformWindowDisplayMode : std::uint8_t
{
  Windowed,
  BorderlessFullscreen,
  ExclusiveFullscreen,
};

struct WindowConfig
{
  std::string title;
  int width = 1280;
  int height = 720;
  bool vsyncEnabled = true;
};

using NativeProcAddressLoader = void* (*)(const char* name);

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
  [[nodiscard]] void* nativeHandle() const;
  [[nodiscard]] static NativeProcAddressLoader nativeProcAddressLoader();
  [[nodiscard]] bool setDisplayMode(PlatformWindowDisplayMode mode,
                                    int windowedWidth,
                                    int windowedHeight,
                                    DiagnosticSink& diagnostics = nullDiagnosticSink());
  void setWindowedSize(int width, int height);
  void setVsyncEnabled(bool enabled);

private:
  void* m_handle = nullptr;
  std::vector<Key> m_pressedKeys;
  bool m_ownsGlfw = false;
  PlatformWindowDisplayMode m_displayMode = PlatformWindowDisplayMode::Windowed;
  int m_windowedX = 100;
  int m_windowedY = 100;
  int m_windowedWidth = 1280;
  int m_windowedHeight = 720;
};
