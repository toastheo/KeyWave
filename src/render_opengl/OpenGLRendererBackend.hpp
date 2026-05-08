#pragma once

#include <vector>

#include "platform/Window.hpp"
#include "render/RenderTypes.hpp"
#include "render/RendererBackend.hpp"
#include "render/RendererView.hpp"
#include "render_opengl/OpenGLShader.hpp"

class OpenGLRendererBackend final : public RendererBackend
{
public:
  explicit OpenGLRendererBackend(NativeProcAddressLoader procAddressLoader, Color clearColor);
  ~OpenGLRendererBackend() override;

  OpenGLRendererBackend(const OpenGLRendererBackend&) = delete;
  OpenGLRendererBackend& operator=(const OpenGLRendererBackend&) = delete;

  bool initialize() override;
  void shutdown() override;
  void setFramebufferSize(const FramebufferSize& size) override;
  void setView(const RendererView& view) override;
  void beginFrame() override;
  void submit(const std::vector<RenderCommand>& commands) override;
  void endFrame() override;

private:
  struct RectVertex
  {
    float x = 0.0f;
    float y = 0.0f;
    float r = 0.0f;
    float g = 0.0f;
    float b = 0.0f;
    float a = 1.0f;
  };

  NativeProcAddressLoader m_procAddressLoader = nullptr;
  Color m_clearColor;
  FramebufferSize m_framebufferSize;
  RendererView m_view;
  OpenGLShader m_rectShader;
  unsigned int m_rectVertexArray = 0;
  unsigned int m_rectVertexBuffer = 0;
  std::vector<RectVertex> m_rectVertices;
  bool m_initialized = false;
};
