#pragma once

#include <span>

#include "render/RenderCommand.hpp"
#include "render/RendererView.hpp"

class RendererBackend
{
public:
  virtual ~RendererBackend() = default;

  virtual bool initialize() = 0;
  virtual void shutdown() = 0;
  virtual void setFramebufferSize(const FramebufferSize& size) = 0;
  virtual void setClearColor(Color color) = 0;
  virtual void setView(const RendererView& view) = 0;
  virtual void beginFrame() = 0;
  virtual void submit(std::span<const RenderCommand> commands) = 0;
  virtual void endFrame() = 0;
};
