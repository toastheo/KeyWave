#pragma once

#include <cstdint>

enum class TransportControlsWindowFlag : std::uint32_t
{
  NoDecoration = 1U << 0U,
  AlwaysAutoResize = 1U << 1U,
  NoSavedSettings = 1U << 2U,
  NoMove = 1U << 3U,
  NoFocusOnAppearing = 1U << 4U,
  NoNavFocus = 1U << 5U,
  NoMouseInputs = 1U << 6U,
};

using TransportControlsWindowFlags = std::uint32_t;

[[nodiscard]] constexpr TransportControlsWindowFlags transportControlsWindowFlagValue(
  const TransportControlsWindowFlag flag)
{
  return static_cast<TransportControlsWindowFlags>(flag);
}

[[nodiscard]] constexpr bool hasTransportControlsWindowFlag(
  const TransportControlsWindowFlags flags, const TransportControlsWindowFlag flag)
{
  return (flags & transportControlsWindowFlagValue(flag)) != 0U;
}

[[nodiscard]] TransportControlsWindowFlags transportControlsWindowFlags();
