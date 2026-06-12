#pragma once

#include "diagnostics/Diagnostics.hpp"

struct GLFWwindow;

class ImGuiLayer
{
public:
  ImGuiLayer() = default;
  ~ImGuiLayer();

  ImGuiLayer(const ImGuiLayer&) = delete;
  ImGuiLayer& operator=(const ImGuiLayer&) = delete;
  ImGuiLayer(ImGuiLayer&&) = delete;
  ImGuiLayer& operator=(ImGuiLayer&&) = delete;

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
