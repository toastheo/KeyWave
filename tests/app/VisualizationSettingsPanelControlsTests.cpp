#include <catch2/catch_test_macros.hpp>

#include "app/VisualizationSettingsPanelControls.hpp"
#include "input/Key.hpp"

namespace {

TEST_CASE("Visualization settings panel controls toggle visibility with escape",
          "[app][visualization-settings]")
{
  bool visible = true;

  applyVisualizationSettingsPanelControl(Key::Escape, visible);
  CHECK_FALSE(visible);

  applyVisualizationSettingsPanelControl(Key::Escape, visible);
  CHECK(visible);
}

TEST_CASE("Visualization settings panel controls ignore other keys",
          "[app][visualization-settings]")
{
  bool visible = true;

  applyVisualizationSettingsPanelControl(Key::Space, visible);

  CHECK(visible);
}

} // namespace
