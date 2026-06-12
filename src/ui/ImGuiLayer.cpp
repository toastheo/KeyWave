#include "ui/ImGuiLayer.hpp"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

bool ImGuiLayer::initialize(GLFWwindow* nativeWindowHandle, DiagnosticSink& diagnostics)
{
  if (m_initialized) {
    return true;
  }

  if (nativeWindowHandle == nullptr) {
    reportError(diagnostics, "ImGui initialization failed: missing native window handle.");
    return false;
  }

  IMGUI_CHECKVERSION();
  ImGui::CreateContext();

  ImGuiIO& io = ImGui::GetIO();
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

  ImGui::StyleColorsDark();

  if (!ImGui_ImplGlfw_InitForOpenGL(nativeWindowHandle, true)) {
    reportError(diagnostics, "ImGui initialization failed: GLFW backend could not initialize.");
    shutdown();
    return false;
  }
  m_glfwBackendInitialized = true;

  if (!ImGui_ImplOpenGL3_Init("#version 330")) {
    reportError(diagnostics, "ImGui initialization failed: OpenGL3 backend could not initialize.");
    shutdown();
    return false;
  }
  m_openglBackendInitialized = true;

  m_initialized = true;
  return true;
}

void ImGuiLayer::shutdown()
{
  if (m_openglBackendInitialized) {
    ImGui_ImplOpenGL3_Shutdown();
    m_openglBackendInitialized = false;
  }

  if (m_glfwBackendInitialized) {
    ImGui_ImplGlfw_Shutdown();
    m_glfwBackendInitialized = false;
  }

  if (ImGui::GetCurrentContext() != nullptr) {
    ImGui::DestroyContext();
  }

  m_initialized = false;
}

void ImGuiLayer::beginFrame() const
{
  if (!m_initialized) {
    return;
  }

  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplGlfw_NewFrame();
  ImGui::NewFrame();
}

void ImGuiLayer::endFrame() const
{
  if (!m_initialized) {
    return;
  }

  ImGui::Render();
  ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

bool ImGuiLayer::wantsKeyboardCapture() const
{
  if (!m_initialized || ImGui::GetCurrentContext() == nullptr) {
    return false;
  }

  return ImGui::GetIO().WantCaptureKeyboard;
}
