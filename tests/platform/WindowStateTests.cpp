#include <catch2/catch_test_macros.hpp>

#include "platform/Window.hpp"

namespace {

TEST_CASE("Maximized native window is restored before windowed size changes",
          "[platform][window]")
{
  CHECK(shouldRestoreNativeWindowBeforeChangingDisplayMode(
    PlatformWindowDisplayMode::Windowed, true));
}

TEST_CASE("Non-windowed or unmaximized windows do not require native restore",
          "[platform][window]")
{
  CHECK_FALSE(shouldRestoreNativeWindowBeforeChangingDisplayMode(
    PlatformWindowDisplayMode::Windowed, false));
  CHECK_FALSE(shouldRestoreNativeWindowBeforeChangingDisplayMode(
    PlatformWindowDisplayMode::BorderlessFullscreen, true));
  CHECK_FALSE(shouldRestoreNativeWindowBeforeChangingDisplayMode(
    PlatformWindowDisplayMode::ExclusiveFullscreen, true));
}

} // namespace
