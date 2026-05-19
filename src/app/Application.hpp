#pragma once

#include <memory>
#include <optional>

#include "app/AppConfig.hpp"
#include "app/AppSettings.hpp"
#include "midi/MidiTimeline.hpp"
#include "platform/Window.hpp"
#include "playback/PlaybackTransport.hpp"
#include "render/RendererBackend.hpp"
#include "ui/ImGuiLayer.hpp"
#include "ui/TransportControls.hpp"

class Application
{
public:
  explicit Application(AppConfig config = {});
  ~Application();

  Application(const Application&) = delete;
  Application& operator=(const Application&) = delete;

  bool initialize();
  void run();
  void shutdown();

private:
  AppConfig m_config;
  AppSettings m_settings;
  Window m_window;
  std::unique_ptr<RendererBackend> m_renderer;
  ImGuiLayer m_imguiLayer;
  TransportControls m_transportControls;
  std::optional<MidiTimeline> m_timeline;
  PlaybackTransport m_playbackTransport;
  bool m_initialized = false;
};
