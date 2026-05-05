#include "render_opengl/OpenGLRendererBackend.hpp"

#include <glad/glad.h>

#include <iostream>

OpenGLRendererBackend::OpenGLRendererBackend(const NativeProcAddressLoader procAddressLoader, const Color clearColor)
  : m_procAddressLoader(procAddressLoader),
    m_clearColor(clearColor) {
}

OpenGLRendererBackend::~OpenGLRendererBackend() {
  shutdown();
}

bool OpenGLRendererBackend::initialize() {
  if (m_initialized) {
    return true;
  }

  if (m_procAddressLoader == nullptr) {
    std::cerr << "OpenGL renderer initialization failed: missing OpenGL procedure loader.\n";
    return false;
  }

  if (gladLoadGLLoader(reinterpret_cast<GLADloadproc>(m_procAddressLoader)) == 0) {
    std::cerr << "OpenGL renderer initialization failed: GLAD could not load OpenGL functions.\n";
    return false;
  }

  glViewport(0, 0, 1280, 720);

  m_initialized = true;
  return true;
}

void OpenGLRendererBackend::shutdown() {
  m_initialized = false;
}

void OpenGLRendererBackend::beginFrame() {
  glClearColor(m_clearColor.r, m_clearColor.g, m_clearColor.b, m_clearColor.a);
  glClear(GL_COLOR_BUFFER_BIT);
}

void OpenGLRendererBackend::endFrame() {
}
