#include <catch2/catch_test_macros.hpp>

#include "ui/TransportTime.hpp"

TEST_CASE("formatTransportTime shows minutes and fractional seconds", "[ui][transport]")
{
  CHECK(formatTransportTime(0.0) == "00:00.00");
  CHECK(formatTransportTime(65.5) == "01:05.50");
  CHECK(formatTransportTime(3723.2) == "62:03.20");
}

TEST_CASE("clampTransportPosition limits slider time to loaded duration", "[ui][transport]")
{
  CHECK(clampTransportPosition(-2.0, 120.0) == 0.0);
  CHECK(clampTransportPosition(30.0, 120.0) == 30.0);
  CHECK(clampTransportPosition(130.0, 120.0) == 120.0);
  CHECK(clampTransportPosition(30.0, 0.0) == 0.0);
}
