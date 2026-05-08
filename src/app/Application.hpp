#pragma once

#include <memory>

#include "app/AppConfig.hpp"
#include "platform/Window.hpp"
#include "render/RenderScene.hpp"
#include "render/RendererBackend.hpp"

class Application
{
public:
  explicit Application(AppConfig config = {});
  ~Application();

  Application(const Application&) = delete;
  Application& operator=(const Application&) = delete;

  bool initialize();
  void run() const;
  void shutdown();

private:
  AppConfig m_config;
  Window m_window;
  std::unique_ptr<RendererBackend> m_renderer;
  RenderScene m_startupScene;
  bool m_initialized = false;
};
