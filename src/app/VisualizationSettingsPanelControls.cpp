#include "app/VisualizationSettingsPanelControls.hpp"

void applyVisualizationSettingsPanelControl(const Key key, bool& visible)
{
  if (key == Key::Escape) {
    visible = !visible;
  }
}
