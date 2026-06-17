#include <catch2/catch_test_macros.hpp>
#include <type_traits>
#include <utility>

#include "platform/Window.hpp"

namespace {

static_assert(std::is_same_v<decltype(std::declval<const Window&>().nativeHandle()), GLFWwindow*>);

TEST_CASE("Maximized native window is restored before windowed size changes", "[platform][window]")
{
  CHECK(
    shouldRestoreNativeWindowBeforeChangingDisplayMode(PlatformWindowDisplayMode::Windowed, true));
}

TEST_CASE("Non-windowed or unmaximized windows do not require native restore", "[platform][window]")
{
  CHECK_FALSE(
    shouldRestoreNativeWindowBeforeChangingDisplayMode(PlatformWindowDisplayMode::Windowed, false));
  CHECK_FALSE(shouldRestoreNativeWindowBeforeChangingDisplayMode(
    PlatformWindowDisplayMode::BorderlessFullscreen, true));
  CHECK_FALSE(shouldRestoreNativeWindowBeforeChangingDisplayMode(
    PlatformWindowDisplayMode::ExclusiveFullscreen, true));
}

} // namespace
