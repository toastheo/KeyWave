#include "keyboard/KeyboardLayout.hpp"

#include "core/CoreTypes.hpp"
#include "keyboard/KeyboardGeometry.hpp"
#include "keyboard/KeyboardState.hpp"
#include "keyboard/KeyboardTypes.hpp"

namespace {

bool isValidKeyRect(const Rect& rect)
{
  return rect.width > 0.0 && rect.height > 0.0;
}

} // namespace

KeyboardLayoutResult KeyboardLayout::build(const KeyboardGeometry& geometry,
                                           const KeyboardState& state)
{
  KeyboardLayoutResult result{
    .width = geometry.width(),
    .height = geometry.height(),
  };

  const auto pitchRange = geometry.config().pitchRange;
  for (auto pitch = pitchRange.minPitch; pitch <= pitchRange.maxPitch; ++pitch) {
    const auto rect = geometry.keyRectForPitch(pitch);
    if (!isValidKeyRect(rect)) {
      continue;
    }

    if (KeyboardGeometry::isWhiteKey(pitch)) {
      result.whiteKeys.push_back(PianoKeyLayout{
        .rect = rect,
        .active = state.isActive(pitch),
      });
      continue;
    }

    result.blackKeys.push_back(PianoKeyLayout{
      .rect = rect,
      .active = state.isActive(pitch),
    });
  }

  return result;
}
