#pragma once

#include <vector>

#include "render/RenderCommand.hpp"
#include "render/RendererView.hpp"

struct RenderScene
{
  std::vector<RenderCommand> commands;
  RendererView view;
};
