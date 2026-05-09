#pragma once

#include <string>
#include <vector>

#include "input/Key.hpp"
#include "render/RenderTypes.hpp"

struct WindowConfig
{
  std::string title;
  int width = 1280;
  int height = 720;
};

using NativeProcAddressLoader = void* (*)(const char* name);

class Window
{
public:
  Window() = default;
  ~Window();

  Window(const Window&) = delete;
  Window& operator=(const Window&) = delete;

  bool initialize(const WindowConfig& config);
  void shutdown();

  [[nodiscard]] bool shouldClose() const;

  static void pollEvents();
  [[nodiscard]] std::vector<Key> consumePressedKeys();
  void swapBuffers() const;
  [[nodiscard]] FramebufferSize framebufferSize() const;
  [[nodiscard]] void* nativeHandle() const;
  [[nodiscard]] static NativeProcAddressLoader nativeProcAddressLoader();

private:
  void* m_handle = nullptr;
  std::vector<Key> m_pressedKeys;
  bool m_ownsGlfw = false;
};
