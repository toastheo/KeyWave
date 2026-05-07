#include "render_opengl/OpenGLRendererBackend.hpp"

#include <glad/glad.h>

#include <cstddef>
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

float mapToClipX(const double x, const Rect& worldViewRect)
{
  return static_cast<float>(((x - worldViewRect.x) / worldViewRect.width * 2.0) - 1.0);
}

float mapToClipY(const double y, const Rect& worldViewRect)
{
  return static_cast<float>(((y - worldViewRect.y) / worldViewRect.height * 2.0) - 1.0);
}

} // namespace

OpenGLRendererBackend::OpenGLRendererBackend(const NativeProcAddressLoader procAddressLoader,
                                             const Color clearColor)
  : m_procAddressLoader(procAddressLoader),
    m_clearColor(clearColor)
{
}

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

  glViewport(0, 0, 1280, 720);

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
  glVertexAttribPointer(0,
                        2,
                        GL_FLOAT,
                        GL_FALSE,
                        static_cast<int>(sizeof(RectVertex)),
                        nullptr);

  glEnableVertexAttribArray(1);
  glVertexAttribPointer(1,
                        4,
                        GL_FLOAT,
                        GL_FALSE,
                        static_cast<int>(sizeof(RectVertex)),
                        colorAttributeOffset);

  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);

  m_initialized = true;
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
    if (!std::holds_alternative<DrawRectCommand>(command)) {
      continue;
    }

    const auto& [rect, color] = std::get<DrawRectCommand>(command);
    if (!isValidRect(rect)) {
      continue;
    }

    const auto left = mapToClipX(rect.x, m_worldViewRect);
    const auto right = mapToClipX(rect.x + rect.width, m_worldViewRect);
    const auto bottom = mapToClipY(rect.y, m_worldViewRect);
    const auto top = mapToClipY(rect.y + rect.height, m_worldViewRect);

    const auto bottomLeft =
      RectVertex{.x = left, .y = bottom, .r = color.r, .g = color.g, .b = color.b, .a = color.a};
    const auto bottomRight =
      RectVertex{.x = right, .y = bottom, .r = color.r, .g = color.g, .b = color.b, .a = color.a};
    const auto topLeft =
      RectVertex{.x = left, .y = top, .r = color.r, .g = color.g, .b = color.b, .a = color.a};
    const auto topRight =
      RectVertex{.x = right, .y = top, .r = color.r, .g = color.g, .b = color.b, .a = color.a};

    m_rectVertices.push_back(bottomLeft);
    m_rectVertices.push_back(bottomRight);
    m_rectVertices.push_back(topRight);

    m_rectVertices.push_back(bottomLeft);
    m_rectVertices.push_back(topRight);
    m_rectVertices.push_back(topLeft);
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
