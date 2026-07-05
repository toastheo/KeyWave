#include "ui/TransportControlsConfig.hpp"

#include <imgui.h>

ImGuiWindowFlags transportControlsWindowFlags()
{
  return ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize |
         ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoMove |
         ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNavFocus;
}
