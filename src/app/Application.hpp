#pragma once

#include <memory>
#include <optional>

#include "app/AppConfig.hpp"
#include "midi/MidiTimeline.hpp"
#include "platform/Window.hpp"
#include "playback/PlaybackTransport.hpp"
#include "render/RendererBackend.hpp"

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
  Window m_window;
  std::unique_ptr<RendererBackend> m_renderer;
  std::optional<MidiTimeline> m_timeline;
  PlaybackTransport m_playbackTransport;
  bool m_initialized = false;
};
