#pragma once

#include <string>

[[nodiscard]] std::string formatTransportTime(double seconds);
[[nodiscard]] double clampTransportPosition(double seconds, double durationSeconds);
