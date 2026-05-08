#pragma once

#include <vector>

#include "render/RenderCommand.hpp"
#include "render/RendererView.hpp"

struct RenderScene
{
  std::vector<RenderCommand> commands;
  RendererView view;

  [[nodiscard]] bool empty() const
  {
    return commands.empty();
  }
};
