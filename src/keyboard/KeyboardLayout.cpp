#include "keyboard/KeyboardLayout.hpp"

namespace {

bool isValidKeyRect(const Rect& rect)
{
  return rect.width > 0.0 && rect.height > 0.0;
}

} // namespace

bool KeyboardLayoutResult::empty() const
{
  return whiteKeys.empty() && blackKeys.empty();
}

KeyboardLayoutResult KeyboardLayout::build(const KeyboardGeometry& geometry)
{
  KeyboardLayoutResult result{
    .pitchRange = geometry.config().pitchRange,
    .width = geometry.width(),
    .height = geometry.height(),
  };

  for (auto pitch = result.pitchRange.minPitch; pitch <= result.pitchRange.maxPitch; ++pitch) {
    const auto rect = geometry.keyRectForPitch(pitch);
    if (!isValidKeyRect(rect)) {
      continue;
    }

    if (KeyboardGeometry::isWhiteKey(pitch)) {
      result.whiteKeys.push_back(PianoKeyLayout{
        .pitch = pitch,
        .kind = PianoKeyKind::White,
        .rect = rect,
      });
      continue;
    }

    result.blackKeys.push_back(PianoKeyLayout{
      .pitch = pitch,
      .kind = PianoKeyKind::Black,
      .rect = rect,
    });
  }

  return result;
}
