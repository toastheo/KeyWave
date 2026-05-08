#include "render_opengl/OpenGLRendererBackend.hpp"

#include <cstddef>
#include <glad/glad.h>
#include <iostream>
#include <string_view>
#include <variant>

namespace {

constexpr std::string_view kRectVertexShaderSource = R"(
#version 330 core

layout (location = 0) in vec2 inPosition;
layout (location = 1) in vec4 inColor;

out vec4 vertexColor;

void main()
{
  vertexColor = inColor;
  gl_Position = vec4(inPosition, 0.0, 1.0);
}
)";

constexpr std::string_view kRectFragmentShaderSource = R"(
#version 330 core

in vec4 vertexColor;

out vec4 outColor;

void main()
{
  outColor = vertexColor;
}
)";

bool isValidRect(const Rect& rect)
{
  return rect.width > 0.0 && rect.height > 0.0;
}

} // namespace

OpenGLRendererBackend::OpenGLRendererBackend(const NativeProcAddressLoader procAddressLoader,
                                             const Color clearColor)
    : m_procAddressLoader(procAddressLoader)
    , m_clearColor(clearColor)
{}

OpenGLRendererBackend::~OpenGLRendererBackend()
{
  shutdown();
}

bool OpenGLRendererBackend::initialize()
{
  if (m_initialized) {
    return true;
  }

  if (m_procAddressLoader == nullptr) {
    std::cerr << "OpenGL renderer initialization failed: missing OpenGL procedure loader.\n";
    return false;
  }

  if (gladLoadGLLoader(m_procAddressLoader) == 0) {
    std::cerr << "OpenGL renderer initialization failed: GLAD could not load OpenGL functions.\n";
    return false;
  }

  if (!m_rectShader.create(OpenGLShaderSources{
        .vertex = kRectVertexShaderSource,
        .fragment = kRectFragmentShaderSource,
      })) {
    std::cerr << "OpenGL renderer initialization failed: rectangle shader could not be created.\n";
    shutdown();
    return false;
  }

  glGenVertexArrays(1, &m_rectVertexArray);
  glGenBuffers(1, &m_rectVertexBuffer);

  if (m_rectVertexArray == 0 || m_rectVertexBuffer == 0) {
    std::cerr << "OpenGL renderer initialization failed: rectangle buffers could not be created.\n";
    shutdown();
    return false;
  }

  glBindVertexArray(m_rectVertexArray);
  glBindBuffer(GL_ARRAY_BUFFER, m_rectVertexBuffer);
  glBufferData(GL_ARRAY_BUFFER, 0, nullptr, GL_DYNAMIC_DRAW);

  constexpr auto kColorAttributeOffset = offsetof(RectVertex, r);
  // OpenGL 3.3 interprets this pointer parameter as a VBO byte offset.
  // NOLINTNEXTLINE(performance-no-int-to-ptr)
  const auto* colorAttributeOffset = reinterpret_cast<const void*>(kColorAttributeOffset);

  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, static_cast<int>(sizeof(RectVertex)), nullptr);

  glEnableVertexAttribArray(1);
  glVertexAttribPointer(
    1, 4, GL_FLOAT, GL_FALSE, static_cast<int>(sizeof(RectVertex)), colorAttributeOffset);

  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);

  m_initialized = true;
  if (isValid(m_framebufferSize)) {
    glViewport(0, 0, m_framebufferSize.width, m_framebufferSize.height);
  }

  return true;
}

void OpenGLRendererBackend::shutdown()
{
  m_rectVertices.clear();

  if (m_rectVertexBuffer != 0) {
    glDeleteBuffers(1, &m_rectVertexBuffer);
    m_rectVertexBuffer = 0;
  }

  if (m_rectVertexArray != 0) {
    glDeleteVertexArrays(1, &m_rectVertexArray);
    m_rectVertexArray = 0;
  }

  m_rectShader.destroy();
  m_initialized = false;
}

void OpenGLRendererBackend::setFramebufferSize(const FramebufferSize& size)
{
  if (!isValid(size)) {
    return;
  }

  if (m_framebufferSize.width == size.width && m_framebufferSize.height == size.height) {
    return;
  }

  m_framebufferSize = size;
  if (m_initialized) {
    glViewport(0, 0, size.width, size.height);
  }
}

void OpenGLRendererBackend::setView(const RendererView& view)
{
  if (!isValid(view.visibleWorldRect)) {
    std::cerr << "OpenGL renderer ignored invalid visible world rectangle"
              << " (x=" << view.visibleWorldRect.x << ", y=" << view.visibleWorldRect.y
              << ", width=" << view.visibleWorldRect.width
              << ", height=" << view.visibleWorldRect.height << ").\n";
    return;
  }

  m_view = view;
}

void OpenGLRendererBackend::appendRectVertices(const Rect& rect, const Color& color)
{
  if (!isValidRect(rect)) {
    return;
  }

  const auto bottomLeftClip = worldToClip(Vec2{.x = rect.x, .y = rect.y}, m_view.visibleWorldRect);
  const auto bottomRightClip =
    worldToClip(Vec2{.x = rect.x + rect.width, .y = rect.y}, m_view.visibleWorldRect);
  const auto topLeftClip =
    worldToClip(Vec2{.x = rect.x, .y = rect.y + rect.height}, m_view.visibleWorldRect);
  const auto topRightClip =
    worldToClip(Vec2{.x = rect.x + rect.width, .y = rect.y + rect.height}, m_view.visibleWorldRect);

  const auto bottomLeft = RectVertex{.x = static_cast<float>(bottomLeftClip.x),
                                     .y = static_cast<float>(bottomLeftClip.y),
                                     .r = color.r,
                                     .g = color.g,
                                     .b = color.b,
                                     .a = color.a};
  const auto bottomRight = RectVertex{.x = static_cast<float>(bottomRightClip.x),
                                      .y = static_cast<float>(bottomRightClip.y),
                                      .r = color.r,
                                      .g = color.g,
                                      .b = color.b,
                                      .a = color.a};
  const auto topLeft = RectVertex{.x = static_cast<float>(topLeftClip.x),
                                  .y = static_cast<float>(topLeftClip.y),
                                  .r = color.r,
                                  .g = color.g,
                                  .b = color.b,
                                  .a = color.a};
  const auto topRight = RectVertex{.x = static_cast<float>(topRightClip.x),
                                   .y = static_cast<float>(topRightClip.y),
                                   .r = color.r,
                                   .g = color.g,
                                   .b = color.b,
                                   .a = color.a};

  m_rectVertices.push_back(bottomLeft);
  m_rectVertices.push_back(bottomRight);
  m_rectVertices.push_back(topRight);

  m_rectVertices.push_back(bottomLeft);
  m_rectVertices.push_back(topRight);
  m_rectVertices.push_back(topLeft);
}

void OpenGLRendererBackend::beginFrame()
{
  if (!m_initialized) {
    return;
  }

  m_rectVertices.clear();
  glClearColor(m_clearColor.r, m_clearColor.g, m_clearColor.b, m_clearColor.a);
  glClear(GL_COLOR_BUFFER_BIT);
}

void OpenGLRendererBackend::submit(const std::vector<RenderCommand>& commands)
{
  if (!m_initialized) {
    return;
  }

  m_rectVertices.reserve(m_rectVertices.size() + commands.size() * 6U);

  for (const auto& command : commands) {
    if (std::holds_alternative<DrawRectCommand>(command)) {
      const auto& [rect, color] = std::get<DrawRectCommand>(command);
      appendRectVertices(rect, color);
      continue;
    }

    if (std::holds_alternative<DrawLineCommand>(command)) {
      const auto& line = std::get<DrawLineCommand>(command);
      appendRectVertices(lineToPixelAlignedRect(line, m_view, m_framebufferSize), line.color);
    }
  }
}

void OpenGLRendererBackend::endFrame()
{
  if (!m_initialized || m_rectVertices.empty() || !m_rectShader.valid()) {
    return;
  }

  glUseProgram(m_rectShader.id());
  glBindVertexArray(m_rectVertexArray);
  glBindBuffer(GL_ARRAY_BUFFER, m_rectVertexBuffer);
  glBufferData(GL_ARRAY_BUFFER,
               static_cast<GLsizeiptr>(m_rectVertices.size() * sizeof(RectVertex)),
               m_rectVertices.data(),
               GL_DYNAMIC_DRAW);
  glDrawArrays(GL_TRIANGLES, 0, static_cast<int>(m_rectVertices.size()));
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);
  glUseProgram(0);
}
