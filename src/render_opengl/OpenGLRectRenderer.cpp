#include "render_opengl/OpenGLRectRenderer.hpp"

#include <cmath>
#include <cstddef>
#include <filesystem>
#include <glad/gl.h>

#include "core/CoreTypes.hpp"
#include "diagnostics/Diagnostics.hpp"
#include "render/RendererView.hpp"
#include "render_opengl/OpenGLShader.hpp"

#ifndef KEYWAVE_SHADER_DIR
#define KEYWAVE_SHADER_DIR "."
#endif

namespace {

bool isValidRect(const Rect& rect)
{
  return rect.width > 0.0 && rect.height > 0.0;
}

bool isFinitePoint(const Vec2& point)
{
  return std::isfinite(point.x) && std::isfinite(point.y);
}

std::filesystem::path shaderPath(const char* filename)
{
  return std::filesystem::path{KEYWAVE_SHADER_DIR} / filename;
}

} // namespace

OpenGLRectRenderer::OpenGLRectRenderer(DiagnosticSink& diagnostics)
    : m_diagnostics(diagnostics)
    , m_shader(diagnostics)
{}

OpenGLRectRenderer::~OpenGLRectRenderer()
{
  shutdown();
}

bool OpenGLRectRenderer::initialize()
{
  if (!m_shader.createFromFiles(OpenGLShaderFilePaths{
        .vertex = shaderPath("rect.vert.glsl"),
        .fragment = shaderPath("rect.frag.glsl"),
      })) {
    reportError(m_diagnostics,
                "OpenGL renderer initialization failed: rectangle shader could not be created.");
    shutdown();
    return false;
  }

  glGenVertexArrays(1, &m_vertexArray);
  glGenBuffers(1, &m_vertexBuffer);

  if (m_vertexArray == 0 || m_vertexBuffer == 0) {
    reportError(m_diagnostics,
                "OpenGL renderer initialization failed: rectangle buffers could not be created.");
    shutdown();
    return false;
  }

  glBindVertexArray(m_vertexArray);
  glBindBuffer(GL_ARRAY_BUFFER, m_vertexBuffer);
  glBufferData(GL_ARRAY_BUFFER, 0, nullptr, GL_DYNAMIC_DRAW);

  constexpr auto kColorAttributeOffset = offsetof(Vertex, r);
  // OpenGL 3.3 interprets this pointer parameter as a VBO byte offset.
  // NOLINTNEXTLINE(performance-no-int-to-ptr)
  const auto* colorAttributeOffset = reinterpret_cast<const void*>(kColorAttributeOffset);

  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, static_cast<int>(sizeof(Vertex)), nullptr);

  glEnableVertexAttribArray(1);
  glVertexAttribPointer(
    1, 4, GL_FLOAT, GL_FALSE, static_cast<int>(sizeof(Vertex)), colorAttributeOffset);

  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);

  return true;
}

void OpenGLRectRenderer::shutdown()
{
  m_vertices.clear();

  if (m_vertexBuffer != 0) {
    glDeleteBuffers(1, &m_vertexBuffer);
    m_vertexBuffer = 0;
  }

  if (m_vertexArray != 0) {
    glDeleteVertexArrays(1, &m_vertexArray);
    m_vertexArray = 0;
  }

  m_shader.destroy();
}

void OpenGLRectRenderer::beginFrame()
{
  m_vertices.clear();
}

void OpenGLRectRenderer::reserve(const std::size_t vertexCount)
{
  m_vertices.reserve(vertexCount);
}

std::size_t OpenGLRectRenderer::vertexCount() const
{
  return m_vertices.size();
}

void OpenGLRectRenderer::appendRect(const Rect& rect, const Color& color, const RendererView& view)
{
  if (!isValidRect(rect)) {
    return;
  }

  const auto bottomLeftClip = worldToClip(Vec2{.x = rect.x, .y = rect.y}, view.visibleWorldRect);
  const auto bottomRightClip =
    worldToClip(Vec2{.x = rect.x + rect.width, .y = rect.y}, view.visibleWorldRect);
  const auto topLeftClip =
    worldToClip(Vec2{.x = rect.x, .y = rect.y + rect.height}, view.visibleWorldRect);
  const auto topRightClip =
    worldToClip(Vec2{.x = rect.x + rect.width, .y = rect.y + rect.height}, view.visibleWorldRect);

  const auto bottomLeft = Vertex{.x = static_cast<float>(bottomLeftClip.x),
                                 .y = static_cast<float>(bottomLeftClip.y),
                                 .r = color.r,
                                 .g = color.g,
                                 .b = color.b,
                                 .a = color.a};
  const auto bottomRight = Vertex{.x = static_cast<float>(bottomRightClip.x),
                                  .y = static_cast<float>(bottomRightClip.y),
                                  .r = color.r,
                                  .g = color.g,
                                  .b = color.b,
                                  .a = color.a};
  const auto topLeft = Vertex{.x = static_cast<float>(topLeftClip.x),
                              .y = static_cast<float>(topLeftClip.y),
                              .r = color.r,
                              .g = color.g,
                              .b = color.b,
                              .a = color.a};
  const auto topRight = Vertex{.x = static_cast<float>(topRightClip.x),
                               .y = static_cast<float>(topRightClip.y),
                               .r = color.r,
                               .g = color.g,
                               .b = color.b,
                               .a = color.a};

  m_vertices.push_back(bottomLeft);
  m_vertices.push_back(bottomRight);
  m_vertices.push_back(topRight);

  m_vertices.push_back(bottomLeft);
  m_vertices.push_back(topRight);
  m_vertices.push_back(topLeft);
}

void OpenGLRectRenderer::appendTriangle(
  const Vec2 a, const Vec2 b, const Vec2 c, const Color& color, const RendererView& view)
{
  if (!isFinitePoint(a) || !isFinitePoint(b) || !isFinitePoint(c)) {
    return;
  }

  const auto aClip = worldToClip(a, view.visibleWorldRect);
  const auto bClip = worldToClip(b, view.visibleWorldRect);
  const auto cClip = worldToClip(c, view.visibleWorldRect);

  m_vertices.push_back(Vertex{.x = static_cast<float>(aClip.x),
                              .y = static_cast<float>(aClip.y),
                              .r = color.r,
                              .g = color.g,
                              .b = color.b,
                              .a = color.a});
  m_vertices.push_back(Vertex{.x = static_cast<float>(bClip.x),
                              .y = static_cast<float>(bClip.y),
                              .r = color.r,
                              .g = color.g,
                              .b = color.b,
                              .a = color.a});
  m_vertices.push_back(Vertex{.x = static_cast<float>(cClip.x),
                              .y = static_cast<float>(cClip.y),
                              .r = color.r,
                              .g = color.g,
                              .b = color.b,
                              .a = color.a});
}

void OpenGLRectRenderer::upload() const
{
  if (m_vertices.empty()) {
    return;
  }

  glBindBuffer(GL_ARRAY_BUFFER, m_vertexBuffer);
  glBufferData(GL_ARRAY_BUFFER,
               static_cast<GLsizeiptr>(m_vertices.size() * sizeof(Vertex)),
               m_vertices.data(),
               GL_DYNAMIC_DRAW);
}

void OpenGLRectRenderer::draw(const int first, const int count) const
{
  if (!m_shader.valid() || count <= 0) {
    return;
  }

  glUseProgram(m_shader.id());
  glBindVertexArray(m_vertexArray);
  glDrawArrays(GL_TRIANGLES, first, count);
}
