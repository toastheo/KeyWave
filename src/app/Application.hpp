#pragma once

#include <memory>

#include "app/AppConfig.hpp"
#include "app/SettingsStorage.hpp"
#include "app/VisualizerController.hpp"
#include "diagnostics/Diagnostics.hpp"
#include "platform/Window.hpp"
#include "render/RendererBackend.hpp"
#include "ui/ImGuiLayer.hpp"

class Application
{
public:
  explicit Application(AppConfig config = {});
  Application(AppConfig config, DiagnosticSink& diagnostics);
  ~Application();

  Application(const Application&) = delete;
  Application& operator=(const Application&) = delete;

  bool initialize();
  void run();
  void shutdown();

private:
  AppConfig m_config;
  DiagnosticSink* m_diagnostics = nullptr;
  SettingsStorage m_settingsStorage;
  VisualizerController m_visualizerController;
  Window m_window;
  std::unique_ptr<RendererBackend> m_renderer;
  ImGuiLayer m_imguiLayer;
  bool m_initialized = false;
  bool m_settingsSaved = false;
};
