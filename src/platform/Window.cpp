#include "platform/Window.hpp"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#if defined(_WIN32)
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

#include <functional>
#include <optional>
#include <sstream>
#include <utility>
#include <vector>

#include "core/CoreTypes.hpp"
#include "diagnostics/Diagnostics.hpp"
#include "input/Key.hpp"
#include "render/RenderTypes.hpp"

struct NativeWindowInteraction
{
#if defined(_WIN32)
  HWND handle = nullptr;
  WNDPROC previousWindowProc = nullptr;
  UINT_PTR updateTimerId = 0;
  bool interactiveMoveOrResize = false;
#endif
  std::function<void()> updateCallback;
};

namespace {
#if defined(_WIN32)
constexpr wchar_t interactionPropertyName[] = L"KeyWave.NativeWindowInteraction";
constexpr UINT interactiveUpdateIntervalMilliseconds = 16;

// Windows runs a nested modal message loop while the user moves or resizes a decorated window.
// During that time glfwPollEvents() does not return, so this procedure uses timer messages to keep
// playback and rendering advancing on the window thread.
LRESULT CALLBACK interactiveWindowProc(HWND handle,
                                       const UINT message,
                                       const WPARAM wParam,
                                       const LPARAM lParam)
{
  auto* interaction =
    static_cast<NativeWindowInteraction*>(GetPropW(handle, interactionPropertyName));
  if (interaction == nullptr || interaction->previousWindowProc == nullptr) {
    return DefWindowProcW(handle, message, wParam, lParam);
  }

  switch (message) {
    case WM_ENTERSIZEMOVE:
      interaction->interactiveMoveOrResize = true;
      interaction->updateTimerId = SetTimer(handle,
                                            reinterpret_cast<UINT_PTR>(interaction),
                                            interactiveUpdateIntervalMilliseconds,
                                            nullptr);
      break;
    case WM_TIMER:
      if (wParam == interaction->updateTimerId && interaction->interactiveMoveOrResize &&
          interaction->updateCallback) {
        // Do not poll GLFW events here because this handler already runs inside Windows nested
        // message loop. The callback updates and renders one frame on the window thread.
        interaction->updateCallback();
      }
      break;
    case WM_EXITSIZEMOVE:
      if (interaction->updateTimerId != 0) {
        KillTimer(handle, interaction->updateTimerId);
        interaction->updateTimerId = 0;
      }
      interaction->interactiveMoveOrResize = false;
      break;
    default:
      break;
  }

  return CallWindowProcW(interaction->previousWindowProc, handle, message, wParam, lParam);
}

std::unique_ptr<NativeWindowInteraction> createNativeWindowInteraction(GLFWwindow* window)
{
  auto interaction = std::make_unique<NativeWindowInteraction>();
  interaction->handle = glfwGetWin32Window(window);
  if (interaction->handle == nullptr) {
    return interaction;
  }

  interaction->previousWindowProc =
    reinterpret_cast<WNDPROC>(GetWindowLongPtrW(interaction->handle, GWLP_WNDPROC));
  if (interaction->previousWindowProc == nullptr) {
    interaction->handle = nullptr;
    return interaction;
  }

  SetLastError(ERROR_SUCCESS);
  if (!SetPropW(interaction->handle, interactionPropertyName, interaction.get())) {
    interaction->handle = nullptr;
    interaction->previousWindowProc = nullptr;
    return interaction;
  }

  SetLastError(ERROR_SUCCESS);
  const auto previous = reinterpret_cast<WNDPROC>(SetWindowLongPtrW(
    interaction->handle, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(interactiveWindowProc)));
  if (previous == nullptr && GetLastError() != ERROR_SUCCESS) {
    RemovePropW(interaction->handle, interactionPropertyName);
    interaction->handle = nullptr;
    interaction->previousWindowProc = nullptr;
  }
  else if (previous != nullptr) {
    interaction->previousWindowProc = previous;
  }

  return interaction;
}

void destroyNativeWindowInteraction(NativeWindowInteraction& interaction)
{
  if (interaction.handle == nullptr) {
    return;
  }

  if (interaction.updateTimerId != 0) {
    KillTimer(interaction.handle, interaction.updateTimerId);
    interaction.updateTimerId = 0;
  }
  if (interaction.previousWindowProc != nullptr) {
    SetWindowLongPtrW(interaction.handle,
                      GWLP_WNDPROC,
                      reinterpret_cast<LONG_PTR>(interaction.previousWindowProc));
  }
  RemovePropW(interaction.handle, interactionPropertyName);
  interaction.handle = nullptr;
  interaction.previousWindowProc = nullptr;
}
#else
std::unique_ptr<NativeWindowInteraction> createNativeWindowInteraction(GLFWwindow*)
{
  return std::make_unique<NativeWindowInteraction>();
}

void destroyNativeWindowInteraction(NativeWindowInteraction&) {}
#endif

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

NativeProcAddress loadOpenGLProcAddress(const char* name)
{
  return glfwGetProcAddress(name);
}

GLFWmonitor* primaryMonitor(DiagnosticSink& diagnostics)
{
  if (glfwGetPrimaryMonitor() == nullptr) {
    reportError(diagnostics, "Failed to apply fullscreen mode: no primary monitor.");
  }
  return glfwGetPrimaryMonitor();
}

const GLFWvidmode* currentVideoMode(GLFWmonitor* monitor, DiagnosticSink& diagnostics)
{
  const GLFWvidmode* mode = glfwGetVideoMode(monitor);
  if (mode == nullptr) {
    reportError(diagnostics, "Failed to apply fullscreen mode: no primary monitor video mode.");
  }
  return mode;
}

void applyWindowedMode(
  GLFWwindow* window, const int x, const int y, const int width, const int height)
{
  glfwSetWindowAttrib(window, GLFW_DECORATED, GLFW_TRUE);
  glfwSetWindowMonitor(window, nullptr, x, y, width, height, GLFW_DONT_CARE);
}

bool isNativeWindowMaximized(GLFWwindow* window)
{
  return glfwGetWindowAttrib(window, GLFW_MAXIMIZED) == GLFW_TRUE;
}

void restoreNativeWindowBeforeChangingDisplayModeIfNeeded(
  GLFWwindow* window, const PlatformWindowDisplayMode currentMode)
{
  if (shouldRestoreNativeWindowBeforeChangingDisplayMode(currentMode,
                                                         isNativeWindowMaximized(window))) {
    glfwRestoreWindow(window);
  }
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

Window::Window() = default;

void GlfwWindowDeleter::operator()(GLFWwindow* window) const
{
  if (window != nullptr) {
    glfwDestroyWindow(window);
  }
}

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

  m_handle.reset(window);
  m_displayMode = PlatformWindowDisplayMode::Windowed;
  m_windowedWidth = config.width;
  m_windowedHeight = config.height;
  glfwGetWindowPos(window, &m_windowedX, &m_windowedY);
  glfwGetWindowSize(window, &m_windowedWidth, &m_windowedHeight);
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
  glfwSwapInterval(config.vsyncEnabled ? 1 : 0);
  m_nativeInteraction = createNativeWindowInteraction(window);

  return true;
}

void Window::shutdown()
{
  if (m_nativeInteraction != nullptr) {
    destroyNativeWindowInteraction(*m_nativeInteraction);
    m_nativeInteraction.reset();
  }
  m_handle.reset();

  if (m_ownsGlfw) {
    glfwTerminate();
    m_ownsGlfw = false;
  }
}

bool Window::shouldClose() const
{
  return m_handle == nullptr || glfwWindowShouldClose(m_handle.get()) == GLFW_TRUE;
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
    glfwSwapBuffers(m_handle.get());
  }
}

FramebufferSize Window::framebufferSize() const
{
  if (m_handle == nullptr) {
    return {};
  }

  FramebufferSize size;
  glfwGetFramebufferSize(m_handle.get(), &size.width, &size.height);
  return size;
}

GLFWwindow* Window::nativeHandle() const
{
  return m_handle.get();
}

NativeProcAddressLoader Window::nativeProcAddressLoader()
{
  return loadOpenGLProcAddress;
}

bool Window::setDisplayMode(const PlatformWindowDisplayMode mode,
                            const WindowedSize windowedSize,
                            DiagnosticSink& diagnostics)
{
  if (m_handle == nullptr) {
    return false;
  }

  auto* window = m_handle.get();
  const bool wasWindowed = m_displayMode == PlatformWindowDisplayMode::Windowed;

  // Requesting a window resize while the window is maximized leads to unexpected behavior, so we
  // have to check if we have to restore the native window.
  restoreNativeWindowBeforeChangingDisplayModeIfNeeded(window, m_displayMode);
  if (wasWindowed) {
    glfwGetWindowPos(window, &m_windowedX, &m_windowedY);
    glfwGetWindowSize(window, &m_windowedWidth, &m_windowedHeight);
  }

  if (mode == PlatformWindowDisplayMode::Windowed || !wasWindowed) {
    m_windowedWidth = windowedSize.width;
    m_windowedHeight = windowedSize.height;
  }

  switch (mode) {
    case PlatformWindowDisplayMode::Windowed:
      applyWindowedMode(window, m_windowedX, m_windowedY, m_windowedWidth, m_windowedHeight);
      m_displayMode = mode;
      return true;

    case PlatformWindowDisplayMode::BorderlessFullscreen: {
      GLFWmonitor* monitor = primaryMonitor(diagnostics);
      if (monitor == nullptr) {
        applyWindowedMode(window, m_windowedX, m_windowedY, m_windowedWidth, m_windowedHeight);
        m_displayMode = PlatformWindowDisplayMode::Windowed;
        return false;
      }
      const GLFWvidmode* modeInfo = currentVideoMode(monitor, diagnostics);
      if (modeInfo == nullptr) {
        applyWindowedMode(window, m_windowedX, m_windowedY, m_windowedWidth, m_windowedHeight);
        m_displayMode = PlatformWindowDisplayMode::Windowed;
        return false;
      }
      int monitorX = 0;
      int monitorY = 0;
      glfwGetMonitorPos(monitor, &monitorX, &monitorY);
      glfwSetWindowAttrib(window, GLFW_DECORATED, GLFW_FALSE);
      glfwSetWindowMonitor(window,
                           nullptr,
                           monitorX,
                           monitorY,
                           modeInfo->width,
                           modeInfo->height,
                           modeInfo->refreshRate);
      m_displayMode = mode;
      return true;
    }

    case PlatformWindowDisplayMode::ExclusiveFullscreen: {
      GLFWmonitor* monitor = primaryMonitor(diagnostics);
      if (monitor == nullptr) {
        applyWindowedMode(window, m_windowedX, m_windowedY, m_windowedWidth, m_windowedHeight);
        m_displayMode = PlatformWindowDisplayMode::Windowed;
        return false;
      }
      const GLFWvidmode* modeInfo = currentVideoMode(monitor, diagnostics);
      if (modeInfo == nullptr) {
        applyWindowedMode(window, m_windowedX, m_windowedY, m_windowedWidth, m_windowedHeight);
        m_displayMode = PlatformWindowDisplayMode::Windowed;
        return false;
      }
      glfwSetWindowAttrib(window, GLFW_DECORATED, GLFW_TRUE);
      glfwSetWindowMonitor(
        window, monitor, 0, 0, modeInfo->width, modeInfo->height, modeInfo->refreshRate);
      m_displayMode = mode;
      return true;
    }
  }

  return false;
}

void Window::setWindowedSize(const WindowedSize size)
{
  m_windowedWidth = size.width;
  m_windowedHeight = size.height;
  if (m_handle == nullptr) {
    return;
  }

  auto* window = m_handle.get();
  if (m_displayMode == PlatformWindowDisplayMode::Windowed) {
    restoreNativeWindowBeforeChangingDisplayModeIfNeeded(window, m_displayMode);
    glfwSetWindowSize(window, size.width, size.height);
  }
}

void Window::setVsyncEnabled(const bool enabled) const
{
  if (m_handle != nullptr) {
    auto* window = m_handle.get();
    glfwMakeContextCurrent(window);
    glfwSwapInterval(enabled ? 1 : 0);
  }
}

void Window::setInteractiveFrameCallback(std::function<void()> callback)
{
  if (m_nativeInteraction != nullptr) {
    m_nativeInteraction->updateCallback = std::move(callback);
  }
}
