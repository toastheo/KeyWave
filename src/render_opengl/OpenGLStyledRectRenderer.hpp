#pragma once

#include <cstddef>
#include <vector>

#include "diagnostics/Diagnostics.hpp"
#include "render/RenderCommand.hpp"
#include "render/RendererView.hpp"
#include "render_opengl/OpenGLShader.hpp"

class OpenGLStyledRectRenderer final
{
public:
  explicit OpenGLStyledRectRenderer(DiagnosticSink& diagnostics = nullDiagnosticSink());
  ~OpenGLStyledRectRenderer();

  OpenGLStyledRectRenderer(const OpenGLStyledRectRenderer&) = delete;
  OpenGLStyledRectRenderer& operator=(const OpenGLStyledRectRenderer&) = delete;

  [[nodiscard]] bool initialize();
  void shutdown();
  void beginFrame();
  void reserve(std::size_t vertexCount);
  [[nodiscard]] std::size_t vertexCount() const;
  void appendRect(const DrawStyledRectCommand& command,
                  const RendererView& view,
                  const FramebufferSize& framebufferSize);
  void upload() const;
  void draw(int first, int count) const;

private:
  struct Vertex
  {
    float x = 0.0f;
    float y = 0.0f;
    float localX = 0.0f;
    float localY = 0.0f;
    float topR = 0.0f;
    float topG = 0.0f;
    float topB = 0.0f;
    float topA = 1.0f;
    float bottomR = 0.0f;
    float bottomG = 0.0f;
    float bottomB = 0.0f;
    float bottomA = 1.0f;
    float borderR = 0.0f;
    float borderG = 0.0f;
    float borderB = 0.0f;
    float borderA = 1.0f;
    float borderThicknessPixels = 0.0f;
    float rectWidthPixels = 0.0f;
    float rectHeightPixels = 0.0f;
    float topLeftCornerRadiusPixels = 0.0f;
    float topRightCornerRadiusPixels = 0.0f;
    float bottomRightCornerRadiusPixels = 0.0f;
    float bottomLeftCornerRadiusPixels = 0.0f;
  };

  DiagnosticSink& m_diagnostics;
  OpenGLShader m_shader;
  unsigned int m_vertexArray = 0;
  unsigned int m_vertexBuffer = 0;
  std::vector<Vertex> m_vertices;
};
