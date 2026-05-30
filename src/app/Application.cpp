#include "app/Application.hpp"

#include <chrono>
#include <memory>
#include <utility>

#include "app/StartupDataLoader.hpp"
#include "render/RenderScene.hpp"
#include "render_opengl/OpenGLRendererBackend.hpp"
#include "ui/TransportControls.hpp"
#include "ui/VisualizationSettingsPanel.hpp"

Application::Application(AppConfig config)
    : Application(std::move(config), consoleDiagnosticSink())
{}

Application::Application(AppConfig config, DiagnosticSink& diagnostics)
    : m_config(std::move(config))
    , m_diagnostics(&diagnostics)
    , m_visualizerController(diagnostics)
{}

Application::~Application()
{
  shutdown();
}

bool Application::initialize()
{
  reportInfo(*m_diagnostics, "Settings path: " + m_settingsStorage.path().string());
  if (auto loadedSettings = m_settingsStorage.load(*m_diagnostics); loadedSettings.has_value()) {
    m_visualizerController.setSettings(*loadedSettings);
    reportInfo(*m_diagnostics, "Settings loaded.");
  }
  reportInfo(*m_diagnostics, "Settings will be saved on exit.");

  auto startupData = StartupDataLoader::load(m_config, *m_diagnostics);
  m_visualizerController.setTimeline(std::move(startupData.timeline));
  const auto& settings = m_visualizerController.settings();

  const WindowConfig windowConfig{
    .title = settings.window.title,
    .width = settings.window.width,
    .height = settings.window.height,
  };

  if (!m_window.initialize(windowConfig, *m_diagnostics)) {
    reportError(*m_diagnostics, "Application initialization failed: window could not be created.");
    return false;
  }

  m_renderer = std::make_unique<OpenGLRendererBackend>(Window::nativeProcAddressLoader(),
                                                       settings.renderer.clearColor,
                                                       *m_diagnostics);

  if (!m_renderer->initialize()) {
    reportError(*m_diagnostics,
                "Application initialization failed: renderer could not be initialized.");
    m_renderer.reset();
    m_window.shutdown();
    return false;
  }
  m_renderer->setFramebufferSize(m_window.framebufferSize());

  if (!m_imguiLayer.initialize(m_window.nativeHandle(), *m_diagnostics)) {
    reportError(*m_diagnostics,
                "Application initialization failed: UI layer could not be initialized.");
    m_renderer->shutdown();
    m_renderer.reset();
    m_window.shutdown();
    return false;
  }

  if (m_visualizerController.hasTimeline()) {
    m_visualizerController.playbackTransport().play();
    reportInfo(*m_diagnostics, "Playback started.");
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
    m_visualizerController.handleInput(pressedKeys, imguiWantsKeyboardCapture);

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
    if (m_settingsStorage.save(m_visualizerController.settings(), *m_diagnostics)) {
      reportInfo(*m_diagnostics, "Settings saved: " + m_settingsStorage.path().string());
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
