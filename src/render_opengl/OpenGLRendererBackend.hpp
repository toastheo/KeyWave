#pragma once

#include "platform/Window.hpp"
#include "render/RenderTypes.hpp"
#include "render/RendererBackend.hpp"

class OpenGLRendererBackend final : public RendererBackend
{
public:
  explicit OpenGLRendererBackend(NativeProcAddressLoader procAddressLoader, Color clearColor);
  ~OpenGLRendererBackend() override;

  OpenGLRendererBackend(const OpenGLRendererBackend&) = delete;
  OpenGLRendererBackend& operator=(const OpenGLRendererBackend&) = delete;

  bool initialize() override;
  void shutdown() override;
  void beginFrame() override;
  void endFrame() override;

private:
  NativeProcAddressLoader m_procAddressLoader = nullptr;
  Color m_clearColor;
  bool m_initialized = false;
};
