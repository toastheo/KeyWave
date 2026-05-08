#pragma once

#include "app/AppConfig.hpp"
#include "render/RenderScene.hpp"

class StartupSceneBuilder
{
public:
  [[nodiscard]] static RenderScene build(const AppConfig& config);
};
