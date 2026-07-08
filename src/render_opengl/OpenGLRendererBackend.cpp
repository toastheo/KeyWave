#include "render_opengl/OpenGLRendererBackend.hpp"

#include <cstddef>
#include <glad/gl.h>
#include <span>
#include <sstream>
#include <variant>

#include "core/CoreTypes.hpp"
#include "diagnostics/Diagnostics.hpp"
#include "render/RenderCommand.hpp"
#include "render/RenderTypes.hpp"
#include "render/RendererView.hpp"

namespace {

GLADapiproc loadOpenGLProcAddress(void* userptr, const char* name)
{
  const auto* loader = static_cast<NativeProcAddressLoader*>(userptr);
  return (*loader)(name);
}

} // namespace

OpenGLRendererBackend::OpenGLRendererBackend(const NativeProcAddressLoader procAddressLoader,
                                             const Color clearColor,
                                             DiagnosticSink& diagnostics)
    : m_procAddressLoader(procAddressLoader)
    , m_diagnostics(diagnostics)
    , m_clearColor(clearColor)
    , m_rectRenderer(diagnostics)
    , m_styledRectRenderer(diagnostics)
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
    reportError(m_diagnostics,
                "OpenGL renderer initialization failed: missing OpenGL procedure loader.");
    return false;
  }

  if (gladLoadGLUserPtr(loadOpenGLProcAddress, static_cast<void*>(&m_procAddressLoader)) == 0) {
    reportError(m_diagnostics,
                "OpenGL renderer initialization failed: GLAD could not load OpenGL functions.");
    return false;
  }

  if (!m_rectRenderer.initialize()) {
    shutdown();
    return false;
  }

  if (!m_styledRectRenderer.initialize()) {
    shutdown();
    return false;
  }

  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  m_initialized = true;
  if (isValid(m_framebufferSize)) {
    glViewport(0, 0, m_framebufferSize.width, m_framebufferSize.height);
  }

  return true;
}

void OpenGLRendererBackend::shutdown()
{
  m_vertexBatches.clear();
  m_styledRectRenderer.shutdown();
  m_rectRenderer.shutdown();
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

void OpenGLRendererBackend::setClearColor(const Color color)
{
  m_clearColor = color;
}

void OpenGLRendererBackend::setView(const RendererView& view)
{
  if (!isValid(view.visibleWorldRect)) {
    std::ostringstream message;
    message << "OpenGL renderer ignored invalid visible world rectangle"
            << " (x=" << view.visibleWorldRect.x << ", y=" << view.visibleWorldRect.y
            << ", width=" << view.visibleWorldRect.width
            << ", height=" << view.visibleWorldRect.height << ").";
    reportWarning(m_diagnostics, message.str());
    return;
  }

  m_view = view;
}

void OpenGLRendererBackend::appendVertexBatch(const VertexBatchKind kind,
                                              const std::size_t first,
                                              const std::size_t count)
{
  if (count == 0U) {
    return;
  }

  const auto firstVertex = static_cast<int>(first);
  const auto vertexCount = static_cast<int>(count);
  if (!m_vertexBatches.empty()) {
    auto& previous = m_vertexBatches.back();
    if (previous.kind == kind && previous.first + previous.count == firstVertex) {
      previous.count += vertexCount;
      return;
    }
  }

  m_vertexBatches.push_back(VertexBatch{
    .kind = kind,
    .first = firstVertex,
    .count = vertexCount,
  });
}

void OpenGLRendererBackend::beginFrame()
{
  if (!m_initialized) {
    return;
  }

  m_rectRenderer.beginFrame();
  m_styledRectRenderer.beginFrame();
  m_vertexBatches.clear();
  glClearColor(m_clearColor.r, m_clearColor.g, m_clearColor.b, m_clearColor.a);
  glClear(GL_COLOR_BUFFER_BIT);
}

void OpenGLRendererBackend::submit(const std::span<const RenderCommand> commands)
{
  if (!m_initialized) {
    return;
  }

  m_rectRenderer.reserve(m_rectRenderer.vertexCount() + commands.size() * 6U);
  m_styledRectRenderer.reserve(m_styledRectRenderer.vertexCount() + commands.size() * 6U);

  for (const auto& command : commands) {
    if (std::holds_alternative<DrawRectCommand>(command)) {
      const auto& [rect, color] = std::get<DrawRectCommand>(command);
      const auto first = m_rectRenderer.vertexCount();
      m_rectRenderer.appendRect(rect, color, m_view);
      appendVertexBatch(VertexBatchKind::Rect, first, m_rectRenderer.vertexCount() - first);
      continue;
    }

    if (std::holds_alternative<DrawTriangleCommand>(command)) {
      const auto& triangle = std::get<DrawTriangleCommand>(command);
      const auto first = m_rectRenderer.vertexCount();
      m_rectRenderer.appendTriangle(triangle.a, triangle.b, triangle.c, triangle.color, m_view);
      appendVertexBatch(VertexBatchKind::Rect, first, m_rectRenderer.vertexCount() - first);
      continue;
    }

    if (std::holds_alternative<DrawStyledRectCommand>(command)) {
      const auto first = m_styledRectRenderer.vertexCount();
      m_styledRectRenderer.appendRect(std::get<DrawStyledRectCommand>(command),
                                      m_view,
                                      m_framebufferSize);
      appendVertexBatch(VertexBatchKind::StyledRect,
                        first,
                        m_styledRectRenderer.vertexCount() - first);
      continue;
    }

    if (std::holds_alternative<DrawLineCommand>(command)) {
      const auto& line = std::get<DrawLineCommand>(command);
      const auto first = m_rectRenderer.vertexCount();
      m_rectRenderer.appendRect(lineToPixelAlignedRect(line, m_view, m_framebufferSize),
                                line.color,
                                m_view);
      appendVertexBatch(VertexBatchKind::Rect, first, m_rectRenderer.vertexCount() - first);
    }
  }
}

void OpenGLRendererBackend::endFrame()
{
  if (!m_initialized || m_vertexBatches.empty()) {
    return;
  }

  m_rectRenderer.upload();
  m_styledRectRenderer.upload();

  for (const auto& batch : m_vertexBatches) {
    if (batch.kind == VertexBatchKind::Rect) {
      m_rectRenderer.draw(batch.first, batch.count);
      continue;
    }

    if (batch.kind == VertexBatchKind::StyledRect) {
      m_styledRectRenderer.draw(batch.first, batch.count);
    }
  }

  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);
  glUseProgram(0);
}
