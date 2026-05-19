#pragma once

#include <unordered_map>

#include "app/AppSettings.hpp"
#include "keyboard/KeyboardTypes.hpp"

/**
 * Renderer-independent piano geometry source of truth.
 * Builds a real piano-style key rectangles and shared pitch-to-x note lanes.
 */
class KeyboardGeometry
{
public:
  explicit KeyboardGeometry(const KeyboardLayoutConfig& config = {});

  // returns the visual key rectangle.
  [[nodiscard]] Rect keyRectForPitch(int pitch) const;

  // returns the horizontal landing area used by falling notes.
  [[nodiscard]] Rect noteRectForPitch(int pitch) const;

  [[nodiscard]] bool containsPitch(int pitch) const;
  [[nodiscard]] static bool isBlackKey(int pitch);
  [[nodiscard]] static bool isWhiteKey(int pitch);

  [[nodiscard]] int whiteKeyCount() const;
  [[nodiscard]] double width() const;
  [[nodiscard]] double height() const;

  [[nodiscard]] const KeyboardLayoutConfig& config() const;

private:
  KeyboardLayoutConfig m_config;
  std::unordered_map<int, Rect> m_keyRects;
  std::unordered_map<int, Rect> m_noteRects;
  int m_whiteKeyCount = 0;
  double m_width = 0.0;
};

[[nodiscard]] KeyboardLayoutConfig keyboardLayoutConfigFromSettings(
  const KeyboardSettings& settings, const PitchRange& pitchRange);
