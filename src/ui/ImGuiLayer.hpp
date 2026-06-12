#pragma once

#include "diagnostics/Diagnostics.hpp"

struct GLFWwindow;

class ImGuiLayer
{
public:
  bool initialize(GLFWwindow* nativeWindowHandle,
                  DiagnosticSink& diagnostics = nullDiagnosticSink());
  void shutdown();

  void beginFrame() const;
  void endFrame() const;

  [[nodiscard]] bool wantsKeyboardCapture() const;

private:
  bool m_initialized = false;
  bool m_glfwBackendInitialized = false;
  bool m_openglBackendInitialized = false;
};
