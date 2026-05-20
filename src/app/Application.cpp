#include "app/Application.hpp"

#include <chrono>
#include <iostream>
#include <memory>
#include <utility>

#include "app/PlaybackTransportControls.hpp"
#include "app/StartupDataLoader.hpp"
#include "fallingnotes/FallingNotesSceneBuilder.hpp"
#include "render/RenderScene.hpp"
#include "render_opengl/OpenGLRendererBackend.hpp"

Application::Application(AppConfig config)
    : m_config(std::move(config))
    , m_settings(sanitizeAppSettings(AppSettings{}))
{}

Application::~Application()
{
  shutdown();
}

bool Application::initialize()
{
  auto startupData = StartupDataLoader::load(m_config);
  m_timeline = std::move(startupData.timeline);

  const WindowConfig windowConfig{
    .title = m_settings.window.title,
    .width = m_settings.window.width,
    .height = m_settings.window.height,
  };

  if (!m_window.initialize(windowConfig)) {
    std::cerr << "Application initialization failed: window could not be created.\n";
    return false;
  }

  m_renderer = std::make_unique<OpenGLRendererBackend>(Window::nativeProcAddressLoader(),
                                                       m_settings.renderer.clearColor);

  if (!m_renderer->initialize()) {
    std::cerr << "Application initialization failed: renderer could not be initialized.\n";
    m_renderer.reset();
    m_window.shutdown();
    return false;
  }
  m_renderer->setFramebufferSize(m_window.framebufferSize());

  if (!m_imguiLayer.initialize(m_window.nativeHandle())) {
    std::cerr << "Application initialization failed: UI layer could not be initialized.\n";
    m_renderer->shutdown();
    m_renderer.reset();
    m_window.shutdown();
    return false;
  }

  if (m_timeline.has_value()) {
    m_playbackTransport.play();
    std::cout << "Playback started.\n";
  }

  m_initialized = true;
  return true;
}

void Application::run()
{
  if (!m_initialized) {
    return;
  }

  auto previousFrameTime = std::chrono::steady_clock::now();

  while (!m_window.shouldClose()) {
    const auto currentFrameTime = std::chrono::steady_clock::now();
    const std::chrono::duration<double> elapsed = currentFrameTime - previousFrameTime;
    previousFrameTime = currentFrameTime;

    Window::pollEvents();
    const auto pressedKeys = m_window.consumePressedKeys();
    if (!m_imguiLayer.wantsKeyboardCapture()) {
      for (const auto key : pressedKeys) {
        applyPlaybackTransportControl(
          key, m_playbackTransport, std::cout, m_settings.playbackControls);
      }
    }

    m_renderer->setFramebufferSize(m_window.framebufferSize());
    m_playbackTransport.update(elapsed.count());
    const double durationSeconds = m_timeline.has_value() ? m_timeline->lengthSeconds() : 0.0;

    m_imguiLayer.beginFrame();
    TransportControls::render(m_playbackTransport, durationSeconds, m_settings.playbackControls);
    VisualizationSettingsPanel::render(m_settings, m_playbackTransport);
    m_renderer->setClearColor(m_settings.renderer.clearColor);

    RenderScene scene;
    if (m_timeline.has_value()) {
      scene = FallingNotesSceneBuilder::build(*m_timeline,
                                              m_playbackTransport.currentTimeSeconds(),
                                              m_settings.fallingNotes,
                                              m_settings.keyboard);
    }

    m_renderer->setView(scene.view);
    m_renderer->beginFrame();
    m_renderer->submit(scene.commands);
    m_renderer->endFrame();
    m_imguiLayer.endFrame();
    m_window.swapBuffers();
  }
}

void Application::shutdown()
{
  m_imguiLayer.shutdown();

  if (m_renderer) {
    m_renderer->shutdown();
    m_renderer.reset();
  }

  m_window.shutdown();
  m_timeline.reset();
  m_playbackTransport.stop();
  m_initialized = false;
}
