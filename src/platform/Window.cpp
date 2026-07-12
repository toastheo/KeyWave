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

#include <atomic>
#include <chrono>
#include <functional>
#include <optional>
#include <sstream>
#include <utility>
#include <vector>

#include "core/CoreTypes.hpp"
#include "diagnostics/Diagnostics.hpp"
#include "input/Key.hpp"
#include "render/RenderTypes.hpp"

// Keeps animation running while Win32 owns the window thread inside its modal move/size loop.
// The timer-queue worker only reads the HWND and coalesces sent notifications. Driver lifecycle and
// the frame callback stay on the window thread, where the OpenGL context and ImGui already live.
struct InteractiveFrameDriver
{
#if defined(_WIN32)
  HWND handle = nullptr;
  WNDPROC previousWindowProc = nullptr;
  HANDLE timerQueueTimer = nullptr;
  std::atomic_bool frameNotificationPending = false;
  std::chrono::steady_clock::time_point lastFrameTime;
  bool active = false;
  bool insideMoveSizeLoop = false;
#endif
  std::function<void()> frameCallback;
};

namespace {
#if defined(_WIN32)
constexpr wchar_t interactiveFrameDriverPropertyName[] = L"KeyWave.InteractiveFrameDriver";
constexpr UINT interactiveFrameIntervalMilliseconds = 16;
constexpr UINT interactiveFrameMessage = WM_APP + 1;

void renderInteractiveFrameIfDue(InteractiveFrameDriver& driver)
{
  if (!driver.active || !driver.frameCallback) {
    return;
  }

  const auto now = std::chrono::steady_clock::now();
  const auto frameInterval =
    std::chrono::milliseconds{interactiveFrameIntervalMilliseconds};
  if (driver.lastFrameTime != std::chrono::steady_clock::time_point{} &&
      now - driver.lastFrameTime < frameInterval) {
    return;
  }

  driver.lastFrameTime = now;
  driver.frameCallback();
}

void CALLBACK requestInteractiveFrame(PVOID context, BOOLEAN)
{
  auto& driver = *static_cast<InteractiveFrameDriver*>(context);
  if (driver.frameNotificationPending.exchange(true)) {
    return;
  }

  // The caption loop can defer queued messages, including WM_TIMER. A sent notification is
  // delivered during message retrieval, while SendNotifyMessage lets this worker return at once.
  if (!SendNotifyMessageW(driver.handle, interactiveFrameMessage, 0, 0)) {
    driver.frameNotificationPending.store(false);
  }
}

void startInteractiveFrameTimer(InteractiveFrameDriver& driver)
{
  if (driver.timerQueueTimer != nullptr) {
    return;
  }

  HANDLE timerQueueTimer = nullptr;
  const auto timerCreated = CreateTimerQueueTimer(&timerQueueTimer,
                                                   nullptr,
                                                   requestInteractiveFrame,
                                                   &driver,
                                                   0,
                                                   interactiveFrameIntervalMilliseconds,
                                                   WT_EXECUTEDEFAULT);
  if (timerCreated != 0) {
    driver.timerQueueTimer = timerQueueTimer;
  }
}

void stopInteractiveFrameTimer(InteractiveFrameDriver& driver)
{
  if (driver.timerQueueTimer != nullptr) {
    // Waiting here guarantees that the worker no longer references the driver during teardown.
    DeleteTimerQueueTimer(nullptr, driver.timerQueueTimer, INVALID_HANDLE_VALUE);
    driver.timerQueueTimer = nullptr;
  }
  driver.frameNotificationPending.store(false);
}

void beginInteractiveFrames(InteractiveFrameDriver& driver)
{
  if (!driver.active) {
    driver.active = true;
    driver.lastFrameTime = {};
  }
  startInteractiveFrameTimer(driver);
}

void endInteractiveFrames(InteractiveFrameDriver& driver)
{
  stopInteractiveFrameTimer(driver);
  driver.active = false;
  driver.lastFrameTime = {};
}

LRESULT CALLBACK interactiveFrameWindowProc(HWND handle,
                                            const UINT message,
                                            const WPARAM wParam,
                                            const LPARAM lParam)
{
  auto* driver = static_cast<InteractiveFrameDriver*>(
    GetPropW(handle, interactiveFrameDriverPropertyName));
  if (driver == nullptr || driver->previousWindowProc == nullptr) {
    return DefWindowProcW(handle, message, wParam, lParam);
  }

  switch (message) {
    case WM_NCLBUTTONDOWN:
      if (wParam == HTCAPTION) {
        // Caption tracking starts before WM_ENTERSIZEMOVE and can block even without mouse motion.
        beginInteractiveFrames(*driver);
      }
      break;
    case WM_ENTERSIZEMOVE:
      driver->insideMoveSizeLoop = true;
      beginInteractiveFrames(*driver);
      break;
    case interactiveFrameMessage:
      driver->frameNotificationPending.store(false);
      renderInteractiveFrameIfDue(*driver);
      break;
    case WM_MOVING:
      // Render directly from movement traffic instead of waiting for the next periodic request.
      renderInteractiveFrameIfDue(*driver);
      break;
    case WM_NCLBUTTONUP:
    case WM_LBUTTONUP:
      if (!driver->insideMoveSizeLoop) {
        endInteractiveFrames(*driver);
      }
      break;
    case WM_EXITSIZEMOVE:
      driver->insideMoveSizeLoop = false;
      endInteractiveFrames(*driver);
      break;
    default:
      break;
  }

  return CallWindowProcW(driver->previousWindowProc, handle, message, wParam, lParam);
}

std::unique_ptr<InteractiveFrameDriver> createInteractiveFrameDriver(
  GLFWwindow* window, std::function<void()> frameCallback)
{
  auto driver = std::make_unique<InteractiveFrameDriver>();
  driver->frameCallback = std::move(frameCallback);
  driver->handle = glfwGetWin32Window(window);
  if (driver->handle == nullptr) {
    return nullptr;
  }

  driver->previousWindowProc =
    reinterpret_cast<WNDPROC>(GetWindowLongPtrW(driver->handle, GWLP_WNDPROC));
  if (driver->previousWindowProc == nullptr) {
    return nullptr;
  }

  SetLastError(ERROR_SUCCESS);
  // Keep this state separate from GLFW's user pointer, which belongs to Window input handling.
  if (!SetPropW(driver->handle, interactiveFrameDriverPropertyName, driver.get())) {
    return nullptr;
  }

  SetLastError(ERROR_SUCCESS);
  // Install the property first so even reentrant window messages can resolve the driver.
  const auto previous = reinterpret_cast<WNDPROC>(SetWindowLongPtrW(
    driver->handle, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(interactiveFrameWindowProc)));
  if (previous == nullptr && GetLastError() != ERROR_SUCCESS) {
    RemovePropW(driver->handle, interactiveFrameDriverPropertyName);
    return nullptr;
  }
  else if (previous != nullptr) {
    driver->previousWindowProc = previous;
  }

  return driver;
}

void destroyInteractiveFrameDriver(InteractiveFrameDriver& driver)
{
  if (driver.handle == nullptr) {
    return;
  }

  stopInteractiveFrameTimer(driver);
  if (driver.previousWindowProc != nullptr) {
    // Restore the chain before removing the property or releasing the driver state.
    SetWindowLongPtrW(
      driver.handle, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(driver.previousWindowProc));
  }
  RemovePropW(driver.handle, interactiveFrameDriverPropertyName);
  driver.handle = nullptr;
  driver.previousWindowProc = nullptr;
}
#else
std::unique_ptr<InteractiveFrameDriver> createInteractiveFrameDriver(
  GLFWwindow*, std::function<void()> frameCallback)
{
  auto driver = std::make_unique<InteractiveFrameDriver>();
  driver->frameCallback = std::move(frameCallback);
  return driver;
}

void destroyInteractiveFrameDriver(InteractiveFrameDriver&) {}
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
  auto* monitor = glfwGetPrimaryMonitor();
  if (monitor == nullptr) {
    reportError(diagnostics, "Failed to apply fullscreen mode: no primary monitor.");
  }
  return monitor;
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

  return true;
}

void Window::shutdown()
{
  setInteractiveFrameCallback({});
  m_handle.reset();
  m_pressedKeys.clear();

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

  // GLFW window-size changes behave inconsistently while a native window is maximized.
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

void Window::setInteractiveFrameCallback(std::function<void()> callback,
                                         DiagnosticSink& diagnostics)
{
  if (!callback) {
    if (m_interactiveFrameDriver != nullptr) {
      m_interactiveFrameDriver->frameCallback = {};
      destroyInteractiveFrameDriver(*m_interactiveFrameDriver);
      m_interactiveFrameDriver.reset();
    }
    return;
  }

  if (m_interactiveFrameDriver != nullptr) {
    m_interactiveFrameDriver->frameCallback = std::move(callback);
    return;
  }

  if (m_handle != nullptr) {
    m_interactiveFrameDriver =
      createInteractiveFrameDriver(m_handle.get(), std::move(callback));
    if (m_interactiveFrameDriver == nullptr) {
      reportWarning(diagnostics,
                    "Interactive window updates disabled: native hook could not be installed.");
    }
  }
}
