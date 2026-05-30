#include "platform/Window.hpp"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <optional>
#include <sstream>

namespace {
void reportGlfwError(const char* fallbackMessage, DiagnosticSink& diagnostics)
{
  const char* description = nullptr;
  const int errorCode = glfwGetError(&description);

  if (description != nullptr) {
    std::ostringstream message;
    message << fallbackMessage << " GLFW error " << errorCode << ": " << description;
    reportError(diagnostics, message.str());
  }
  else {
    reportError(diagnostics, fallbackMessage);
  }
}

void* loadOpenGLProcAddress(const char* name)
{
  return reinterpret_cast<void*>(glfwGetProcAddress(name));
}

std::optional<Key> keyFromGlfwKey(const int key)
{
  switch (key) {
    case GLFW_KEY_SPACE:
      return Key::Space;
    case GLFW_KEY_ESCAPE:
      return Key::Escape;
    case GLFW_KEY_R:
      return Key::R;
    case GLFW_KEY_S:
      return Key::S;
    case GLFW_KEY_LEFT:
      return Key::Left;
    case GLFW_KEY_RIGHT:
      return Key::Right;
    case GLFW_KEY_UP:
      return Key::Up;
    case GLFW_KEY_DOWN:
      return Key::Down;
    default:
      return std::nullopt;
  }
}
} // namespace

Window::~Window()
{
  shutdown();
}

bool Window::initialize(const WindowConfig& config, DiagnosticSink& diagnostics)
{
  if (m_handle != nullptr) {
    return true;
  }

  if (glfwInit() != GLFW_TRUE) {
    reportGlfwError("Failed to initialize GLFW.", diagnostics);
    return false;
  }
  m_ownsGlfw = true;

  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#if defined(__APPLE__)
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
#endif

  GLFWwindow* window =
    glfwCreateWindow(config.width, config.height, config.title.c_str(), nullptr, nullptr);
  if (window == nullptr) {
    reportGlfwError("Failed to create the KeyWave window.", diagnostics);
    shutdown();
    return false;
  }

  m_handle = window;
  glfwSetWindowUserPointer(window, this);
  glfwSetKeyCallback(window,
                     [](GLFWwindow* callbackWindow, const int key, int, const int action, int) {
                       if (action != GLFW_PRESS) {
                         return;
                       }

                       auto* owner = static_cast<Window*>(glfwGetWindowUserPointer(callbackWindow));
                       if (owner == nullptr) {
                         return;
                       }

                       if (const auto mappedKey = keyFromGlfwKey(key); mappedKey.has_value()) {
                         owner->m_pressedKeys.push_back(*mappedKey);
                       }
                     });

  glfwMakeContextCurrent(window);
  glfwSwapInterval(1);

  return true;
}

void Window::shutdown()
{
  if (m_handle != nullptr) {
    glfwDestroyWindow(static_cast<GLFWwindow*>(m_handle));
    m_handle = nullptr;
  }

  if (m_ownsGlfw) {
    glfwTerminate();
    m_ownsGlfw = false;
  }
}

bool Window::shouldClose() const
{
  return m_handle == nullptr ||
         glfwWindowShouldClose(static_cast<GLFWwindow*>(m_handle)) == GLFW_TRUE;
}

void Window::pollEvents()
{
  glfwPollEvents();
}

std::vector<Key> Window::consumePressedKeys()
{
  std::vector<Key> pressedKeys;
  pressedKeys.swap(m_pressedKeys);
  return pressedKeys;
}

void Window::swapBuffers() const
{
  if (m_handle != nullptr) {
    glfwSwapBuffers(static_cast<GLFWwindow*>(m_handle));
  }
}

FramebufferSize Window::framebufferSize() const
{
  if (m_handle == nullptr) {
    return {};
  }

  FramebufferSize size;
  glfwGetFramebufferSize(static_cast<GLFWwindow*>(m_handle), &size.width, &size.height);
  return size;
}

void* Window::nativeHandle() const
{
  return m_handle;
}

NativeProcAddressLoader Window::nativeProcAddressLoader()
{
  return loadOpenGLProcAddress;
}
