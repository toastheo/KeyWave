#pragma once

#include <memory>
#include <vector>

#include "app/AppConfig.hpp"
#include "platform/Window.hpp"
#include "render/RenderCommand.hpp"
#include "render/RendererBackend.hpp"
#include "render/RendererView.hpp"

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
  std::vector<RenderCommand> m_startupRenderCommands;
  RendererView m_startupRendererView;
  bool m_initialized = false;
};
