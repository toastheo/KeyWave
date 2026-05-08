#include "app/Application.hpp"

#include <iostream>
#include <memory>
#include <utility>

#include "app/StartupSceneBuilder.hpp"
#include "render_opengl/OpenGLRendererBackend.hpp"

Application::Application(AppConfig config)
  : m_config(std::move(config))
{
}

Application::~Application()
{
  shutdown();
}

bool Application::initialize()
{
  m_startupScene = StartupSceneBuilder::build(m_config);

  const WindowConfig windowConfig{
    .title = "KeyWave",
    .width = 1280,
    .height = 720,
  };

  if (!m_window.initialize(windowConfig)) {
    std::cerr << "Application initialization failed: window could not be created.\n";
    return false;
  }

  m_renderer =
    std::make_unique<OpenGLRendererBackend>(Window::nativeProcAddressLoader(),
                                            Color{.r = 0.025f, .g = 0.03f, .b = 0.04f, .a = 1.0f});

  if (!m_renderer->initialize()) {
    std::cerr << "Application initialization failed: renderer could not be initialized.\n";
    m_renderer.reset();
    m_window.shutdown();
    return false;
  }

  m_renderer->setView(m_startupScene.view);

  m_initialized = true;
  return true;
}

void Application::run() const
{
  if (!m_initialized) {
    return;
  }

  while (!m_window.shouldClose()) {
    Window::pollEvents();
    m_renderer->beginFrame();
    m_renderer->submit(m_startupScene.commands);
    m_renderer->endFrame();
    m_window.swapBuffers();
  }
}

void Application::shutdown()
{
  if (m_renderer) {
    m_renderer->shutdown();
    m_renderer.reset();
  }

  m_window.shutdown();
  m_startupScene = {};
  m_initialized = false;
}
