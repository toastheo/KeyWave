#pragma once

#include "platform/Window.hpp"
#include "render/RendererBackend.hpp"

#include <memory>

class Application {
public:
  Application() = default;
  ~Application();

  Application(const Application&) = delete;
  Application& operator=(const Application&) = delete;

  bool initialize();
  void run();
  void shutdown();

private:
  Window m_window;
  std::unique_ptr<RendererBackend> m_renderer;
  bool m_initialized = false;
};
