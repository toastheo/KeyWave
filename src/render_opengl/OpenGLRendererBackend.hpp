#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

#include "diagnostics/Diagnostics.hpp"
#include "platform/Window.hpp"
#include "render/RenderTypes.hpp"
#include "render/RendererBackend.hpp"
#include "render/RendererView.hpp"
#include "render_opengl/OpenGLRectRenderer.hpp"
#include "render_opengl/OpenGLStyledRectRenderer.hpp"

class OpenGLRendererBackend final : public RendererBackend
{
public:
  OpenGLRendererBackend(NativeProcAddressLoader procAddressLoader,
                        Color clearColor,
                        DiagnosticSink& diagnostics = nullDiagnosticSink());
  ~OpenGLRendererBackend() override;

  OpenGLRendererBackend(const OpenGLRendererBackend&) = delete;
  OpenGLRendererBackend& operator=(const OpenGLRendererBackend&) = delete;

  bool initialize() override;
  void shutdown() override;
  void setFramebufferSize(const FramebufferSize& size) override;
  void setClearColor(Color color) override;
  void setView(const RendererView& view) override;
  void beginFrame() override;
  void submit(std::span<const RenderCommand> commands) override;
  void endFrame() override;

private:
  enum class VertexBatchKind : std::uint8_t
  {
    Rect,
    StyledRect,
  };

  struct VertexBatch
  {
    VertexBatchKind kind = VertexBatchKind::Rect;
    int first = 0;
    int count = 0;
  };

  void appendVertexBatch(VertexBatchKind kind, std::size_t first, std::size_t count);

  NativeProcAddressLoader m_procAddressLoader = nullptr;
  DiagnosticSink& m_diagnostics;
  Color m_clearColor;
  FramebufferSize m_framebufferSize;
  RendererView m_view;
  OpenGLRectRenderer m_rectRenderer;
  OpenGLStyledRectRenderer m_styledRectRenderer;
  std::vector<VertexBatch> m_vertexBatches;
  bool m_initialized = false;
};
