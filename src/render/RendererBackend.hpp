#pragma once

#include <vector>

#include "render/RenderCommand.hpp"
#include "render/RendererView.hpp"

class RendererBackend
{
public:
  virtual ~RendererBackend() = default;

  virtual bool initialize() = 0;
  virtual void shutdown() = 0;
  virtual void setView(const RendererView& view) = 0;
  virtual void beginFrame() = 0;
  virtual void submit(const std::vector<RenderCommand>& commands) = 0;
  virtual void endFrame() = 0;
};
