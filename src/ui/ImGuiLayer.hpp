#pragma once

class ImGuiLayer
{
public:
  bool initialize(void* nativeWindowHandle);
  void shutdown();

  void beginFrame() const;
  void endFrame() const;

  [[nodiscard]] bool wantsKeyboardCapture() const;

private:
  bool m_initialized = false;
  bool m_glfwBackendInitialized = false;
  bool m_openglBackendInitialized = false;
};
