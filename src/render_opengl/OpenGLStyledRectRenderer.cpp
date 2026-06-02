#include "render_opengl/OpenGLStyledRectRenderer.hpp"

#include <cmath>
#include <cstddef>
#include <filesystem>
#include <glad/glad.h>

#ifndef KEYWAVE_SHADER_DIR
#define KEYWAVE_SHADER_DIR "."
#endif

namespace {

bool isValidRect(const Rect& rect)
{
  return rect.width > 0.0 && rect.height > 0.0;
}

Vec2 rectPixelSize(const Rect& rect,
                   const RendererView& view,
                   const FramebufferSize& framebufferSize)
{
  if (!isValid(view.visibleWorldRect) || !isValid(framebufferSize)) {
    return {};
  }

  return Vec2{
    .x = rect.width / view.visibleWorldRect.width * static_cast<double>(framebufferSize.width),
    .y = rect.height / view.visibleWorldRect.height * static_cast<double>(framebufferSize.height),
  };
}

float positiveFiniteFloat(const double value)
{
  if (!std::isfinite(value) || value <= 0.0) {
    return 0.0f;
  }

  return static_cast<float>(value);
}

std::filesystem::path shaderPath(const char* filename)
{
  return std::filesystem::path{KEYWAVE_SHADER_DIR} / filename;
}

} // namespace

OpenGLStyledRectRenderer::OpenGLStyledRectRenderer(DiagnosticSink& diagnostics)
    : m_diagnostics(diagnostics)
    , m_shader(diagnostics)
{}

OpenGLStyledRectRenderer::~OpenGLStyledRectRenderer()
{
  shutdown();
}

bool OpenGLStyledRectRenderer::initialize()
{
  if (!m_shader.createFromFiles(OpenGLShaderFilePaths{
        .vertex = shaderPath("styled_rect.vert.glsl"),
        .fragment = shaderPath("styled_rect.frag.glsl"),
      })) {
    reportError(m_diagnostics,
                "OpenGL renderer initialization failed: styled rectangle shader could not be "
                "created.");
    shutdown();
    return false;
  }

  glGenVertexArrays(1, &m_vertexArray);
  glGenBuffers(1, &m_vertexBuffer);

  if (m_vertexArray == 0 || m_vertexBuffer == 0) {
    reportError(
      m_diagnostics,
      "OpenGL renderer initialization failed: styled rectangle buffers could not be created.");
    shutdown();
    return false;
  }

  glBindVertexArray(m_vertexArray);
  glBindBuffer(GL_ARRAY_BUFFER, m_vertexBuffer);
  glBufferData(GL_ARRAY_BUFFER, 0, nullptr, GL_DYNAMIC_DRAW);

  constexpr auto kLocalAttributeOffset = offsetof(Vertex, localX);
  constexpr auto kTopColorAttributeOffset = offsetof(Vertex, topR);
  constexpr auto kBottomColorAttributeOffset = offsetof(Vertex, bottomR);
  constexpr auto kBorderColorAttributeOffset = offsetof(Vertex, borderR);
  constexpr auto kBorderThicknessAttributeOffset = offsetof(Vertex, borderThicknessPixels);
  constexpr auto kRectSizeAttributeOffset = offsetof(Vertex, rectWidthPixels);
  constexpr auto kCornerRadiiAttributeOffset = offsetof(Vertex, topLeftCornerRadiusPixels);

  // OpenGL 3.3 interprets pointer parameters below as VBO byte offsets.
  // NOLINTBEGIN(performance-no-int-to-ptr)
  const auto* localAttributeOffset = reinterpret_cast<const void*>(kLocalAttributeOffset);
  const auto* topColorAttributeOffset = reinterpret_cast<const void*>(kTopColorAttributeOffset);
  const auto* bottomColorAttributeOffset =
    reinterpret_cast<const void*>(kBottomColorAttributeOffset);
  const auto* borderColorAttributeOffset =
    reinterpret_cast<const void*>(kBorderColorAttributeOffset);
  const auto* borderThicknessAttributeOffset =
    reinterpret_cast<const void*>(kBorderThicknessAttributeOffset);
  const auto* rectSizeAttributeOffset = reinterpret_cast<const void*>(kRectSizeAttributeOffset);
  const auto* cornerRadiiAttributeOffset =
    reinterpret_cast<const void*>(kCornerRadiiAttributeOffset);
  // NOLINTEND(performance-no-int-to-ptr)

  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, static_cast<int>(sizeof(Vertex)), nullptr);

  glEnableVertexAttribArray(1);
  glVertexAttribPointer(
    1, 2, GL_FLOAT, GL_FALSE, static_cast<int>(sizeof(Vertex)), localAttributeOffset);

  glEnableVertexAttribArray(2);
  glVertexAttribPointer(
    2, 4, GL_FLOAT, GL_FALSE, static_cast<int>(sizeof(Vertex)), topColorAttributeOffset);

  glEnableVertexAttribArray(3);
  glVertexAttribPointer(
    3, 4, GL_FLOAT, GL_FALSE, static_cast<int>(sizeof(Vertex)), bottomColorAttributeOffset);

  glEnableVertexAttribArray(4);
  glVertexAttribPointer(
    4, 4, GL_FLOAT, GL_FALSE, static_cast<int>(sizeof(Vertex)), borderColorAttributeOffset);

  glEnableVertexAttribArray(5);
  glVertexAttribPointer(
    5, 1, GL_FLOAT, GL_FALSE, static_cast<int>(sizeof(Vertex)), borderThicknessAttributeOffset);

  glEnableVertexAttribArray(6);
  glVertexAttribPointer(
    6, 2, GL_FLOAT, GL_FALSE, static_cast<int>(sizeof(Vertex)), rectSizeAttributeOffset);

  glEnableVertexAttribArray(7);
  glVertexAttribPointer(
    7, 4, GL_FLOAT, GL_FALSE, static_cast<int>(sizeof(Vertex)), cornerRadiiAttributeOffset);

  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);

  return true;
}

void OpenGLStyledRectRenderer::shutdown()
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

void OpenGLStyledRectRenderer::beginFrame()
{
  m_vertices.clear();
}

void OpenGLStyledRectRenderer::reserve(const std::size_t vertexCount)
{
  m_vertices.reserve(vertexCount);
}

std::size_t OpenGLStyledRectRenderer::vertexCount() const
{
  return m_vertices.size();
}

void OpenGLStyledRectRenderer::appendRect(const DrawStyledRectCommand& command,
                                          const RendererView& view,
                                          const FramebufferSize& framebufferSize)
{
  if (!isValidRect(command.rect)) {
    return;
  }

  const auto bottomLeftClip =
    worldToClip(Vec2{.x = command.rect.x, .y = command.rect.y}, view.visibleWorldRect);
  const auto bottomRightClip =
    worldToClip(Vec2{.x = command.rect.x + command.rect.width, .y = command.rect.y},
                view.visibleWorldRect);
  const auto topLeftClip =
    worldToClip(Vec2{.x = command.rect.x, .y = command.rect.y + command.rect.height},
                view.visibleWorldRect);
  const auto topRightClip = worldToClip(Vec2{.x = command.rect.x + command.rect.width,
                                             .y = command.rect.y + command.rect.height},
                                        view.visibleWorldRect);
  const auto pixelSize = rectPixelSize(command.rect, view, framebufferSize);
  const auto borderThicknessPixels = positiveFiniteFloat(command.borderThicknessPixels);
  const auto topLeftCornerRadiusPixels = positiveFiniteFloat(command.cornerRadiiPixels.topLeft);
  const auto topRightCornerRadiusPixels = positiveFiniteFloat(command.cornerRadiiPixels.topRight);
  const auto bottomRightCornerRadiusPixels =
    positiveFiniteFloat(command.cornerRadiiPixels.bottomRight);
  const auto bottomLeftCornerRadiusPixels =
    positiveFiniteFloat(command.cornerRadiiPixels.bottomLeft);

  const auto vertex = [&](const Vec2& clipPosition, const float localX, const float localY) {
    return Vertex{
      .x = static_cast<float>(clipPosition.x),
      .y = static_cast<float>(clipPosition.y),
      .localX = localX,
      .localY = localY,
      .topR = command.topColor.r,
      .topG = command.topColor.g,
      .topB = command.topColor.b,
      .topA = command.topColor.a,
      .bottomR = command.bottomColor.r,
      .bottomG = command.bottomColor.g,
      .bottomB = command.bottomColor.b,
      .bottomA = command.bottomColor.a,
      .borderR = command.borderColor.r,
      .borderG = command.borderColor.g,
      .borderB = command.borderColor.b,
      .borderA = command.borderColor.a,
      .borderThicknessPixels = borderThicknessPixels,
      .rectWidthPixels = static_cast<float>(pixelSize.x),
      .rectHeightPixels = static_cast<float>(pixelSize.y),
      .topLeftCornerRadiusPixels = topLeftCornerRadiusPixels,
      .topRightCornerRadiusPixels = topRightCornerRadiusPixels,
      .bottomRightCornerRadiusPixels = bottomRightCornerRadiusPixels,
      .bottomLeftCornerRadiusPixels = bottomLeftCornerRadiusPixels,
    };
  };

  const auto bottomLeft = vertex(bottomLeftClip, 0.0f, 0.0f);
  const auto bottomRight = vertex(bottomRightClip, 1.0f, 0.0f);
  const auto topLeft = vertex(topLeftClip, 0.0f, 1.0f);
  const auto topRight = vertex(topRightClip, 1.0f, 1.0f);

  m_vertices.push_back(bottomLeft);
  m_vertices.push_back(bottomRight);
  m_vertices.push_back(topRight);

  m_vertices.push_back(bottomLeft);
  m_vertices.push_back(topRight);
  m_vertices.push_back(topLeft);
}

void OpenGLStyledRectRenderer::upload() const
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

void OpenGLStyledRectRenderer::draw(const int first, const int count) const
{
  if (!m_shader.valid() || count <= 0) {
    return;
  }

  glUseProgram(m_shader.id());
  glBindVertexArray(m_vertexArray);
  glDrawArrays(GL_TRIANGLES, first, count);
}
