#include "app/VisualizationSettingsPanelControls.hpp"

#include "input/Key.hpp"

void applyVisualizationSettingsPanelControl(const Key key, bool& visible)
{
  if (key == Key::Escape) {
    visible = !visible;
  }
}
