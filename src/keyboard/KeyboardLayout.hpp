#pragma once

#include "keyboard/KeyboardGeometry.hpp"

/**
 * Builds keyboard draw layout grouped for painter ordering:
 * white keys first, black keys above them.
 */
class KeyboardLayout
{
public:
  [[nodiscard]] static KeyboardLayoutResult build(const KeyboardGeometry& geometry);
};
