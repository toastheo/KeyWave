#pragma once

#include <cstddef>
#include <vector>

#include "diagnostics/Diagnostics.hpp"
#include "render/RenderTypes.hpp"
#include "render/RendererView.hpp"
#include "render_opengl/OpenGLShader.hpp"

class OpenGLRectRenderer final
{
public:
  explicit OpenGLRectRenderer(DiagnosticSink& diagnostics = nullDiagnosticSink());
  ~OpenGLRectRenderer();

  OpenGLRectRenderer(const OpenGLRectRenderer&) = delete;
  OpenGLRectRenderer& operator=(const OpenGLRectRenderer&) = delete;

  [[nodiscard]] bool initialize();
  void shutdown();
  void beginFrame();
  void reserve(std::size_t vertexCount);
  [[nodiscard]] std::size_t vertexCount() const;
  void appendRect(const Rect& rect, const Color& color, const RendererView& view);
  void appendTriangle(Vec2 a, Vec2 b, Vec2 c, const Color& color, const RendererView& view);
  void upload() const;
  void draw(int first, int count) const;

private:
  struct Vertex
  {
    float x = 0.0f;
    float y = 0.0f;
    float r = 0.0f;
    float g = 0.0f;
    float b = 0.0f;
    float a = 1.0f;
  };

  DiagnosticSink& m_diagnostics;
  OpenGLShader m_shader;
  unsigned int m_vertexArray = 0;
  unsigned int m_vertexBuffer = 0;
  std::vector<Vertex> m_vertices;
};
