#include "app/Application.hpp"

#include <chrono>
#include <iostream>
#include <memory>
#include <utility>

#include "app/StartupDataLoader.hpp"
#include "render/RenderScene.hpp"
#include "render_opengl/OpenGLRendererBackend.hpp"
#include "ui/TransportControls.hpp"
#include "ui/VisualizationSettingsPanel.hpp"

Application::Application(AppConfig config)
    : m_config(std::move(config))
{}

Application::~Application()
{
  shutdown();
}

bool Application::initialize()
{
  std::cout << "Settings path: " << m_settingsStorage.path() << '\n';
  if (auto loadedSettings = m_settingsStorage.load(); loadedSettings.has_value()) {
    m_visualizerController.setSettings(*loadedSettings);
    std::cout << "Settings loaded.\n";
  }
  std::cout << "Settings will be saved on exit.\n";

  auto startupData = StartupDataLoader::load(m_config);
  m_visualizerController.setTimeline(std::move(startupData.timeline));
  const auto& settings = m_visualizerController.settings();

  const WindowConfig windowConfig{
    .title = settings.window.title,
    .width = settings.window.width,
    .height = settings.window.height,
  };

  if (!m_window.initialize(windowConfig)) {
    std::cerr << "Application initialization failed: window could not be created.\n";
    return false;
  }

  m_renderer = std::make_unique<OpenGLRendererBackend>(Window::nativeProcAddressLoader(),
                                                       settings.renderer.clearColor);

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

  if (m_visualizerController.hasTimeline()) {
    m_visualizerController.playbackTransport().play();
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
    const auto imguiWantsKeyboardCapture = m_imguiLayer.wantsKeyboardCapture();
    m_visualizerController.handleInput(pressedKeys, imguiWantsKeyboardCapture, std::cout);

    m_renderer->setFramebufferSize(m_window.framebufferSize());
    m_visualizerController.update(elapsed.count());

    m_imguiLayer.beginFrame();
    TransportControls::render(m_visualizerController.playbackTransport(),
                              m_visualizerController.durationSeconds(),
                              m_visualizerController.settings().playbackControls);
    if (m_visualizerController.visualizationSettingsPanelVisible()) {
      VisualizationSettingsPanel::render(m_visualizerController.settings(),
                                         m_visualizerController.playbackTransport());
    }
    m_renderer->setClearColor(m_visualizerController.settings().renderer.clearColor);

    const auto scene = m_visualizerController.buildScene();

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
  if (!m_settingsSaved) {
    if (m_settingsStorage.save(m_visualizerController.settings())) {
      std::cout << "Settings saved: " << m_settingsStorage.path() << '\n';
    }
    m_settingsSaved = true;
  }

  m_imguiLayer.shutdown();

  if (m_renderer) {
    m_renderer->shutdown();
    m_renderer.reset();
  }

  m_window.shutdown();
  m_visualizerController.setTimeline(std::nullopt);
  m_visualizerController.playbackTransport().stop();
  m_initialized = false;
}
