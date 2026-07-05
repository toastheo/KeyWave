#include <catch2/catch_test_macros.hpp>

#include "ui/TransportControlsConfig.hpp"

TEST_CASE("transport controls do not take initial keyboard focus", "[ui][transport]")
{
  const auto flags = transportControlsWindowFlags();

  CHECK(hasTransportControlsWindowFlag(flags, TransportControlsWindowFlag::NoFocusOnAppearing));
  CHECK(hasTransportControlsWindowFlag(flags, TransportControlsWindowFlag::NoNavFocus));
  CHECK_FALSE(hasTransportControlsWindowFlag(flags, TransportControlsWindowFlag::NoMouseInputs));
}
