#include "ui/TransportTime.hpp"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <sstream>

std::string formatTransportTime(const double seconds)
{
  const auto safeSeconds = std::isfinite(seconds) ? std::max(0.0, seconds) : 0.0;
  const auto totalCentiseconds = std::llround(safeSeconds * 100.0);
  const auto minutes = totalCentiseconds / 6000;
  const auto wholeSeconds = (totalCentiseconds / 100) % 60;
  const auto centiseconds = totalCentiseconds % 100;

  std::ostringstream output;
  output << std::setfill('0') << std::setw(2) << minutes << ':' << std::setw(2) << wholeSeconds
         << '.' << std::setw(2) << centiseconds;
  return output.str();
}

double clampTransportPosition(const double seconds, const double durationSeconds)
{
  if (!std::isfinite(seconds) || !std::isfinite(durationSeconds) || durationSeconds <= 0.0) {
    return 0.0;
  }

  return std::clamp(seconds, 0.0, durationSeconds);
}
