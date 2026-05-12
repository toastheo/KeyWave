#include "ui/TransportControlsConfig.hpp"

#include <catch2/catch_test_macros.hpp>
#include <imgui.h>

TEST_CASE("transport controls do not take initial keyboard focus", "[ui][transport]")
{
  const auto flags = transportControlsWindowFlags();

  CHECK((flags & ImGuiWindowFlags_NoFocusOnAppearing) != 0);
  CHECK((flags & ImGuiWindowFlags_NoNavFocus) != 0);
  CHECK((flags & ImGuiWindowFlags_NoMouseInputs) == 0);
}
