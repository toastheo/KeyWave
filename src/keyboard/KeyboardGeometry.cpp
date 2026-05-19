#include "keyboard/KeyboardGeometry.hpp"

#include <algorithm>
#include <cmath>

namespace {

constexpr double kMinimumPositiveWidth = 0.000001;

int pitchClass(const int pitch)
{
  const auto remainder = pitch % 12;
  return remainder < 0 ? remainder + 12 : remainder;
}

bool isBlackPitchClass(const int pitchClass)
{
  return pitchClass == 1 || pitchClass == 3 || pitchClass == 6 || pitchClass == 8 ||
         pitchClass == 10;
}

bool isWhitePitchClass(const int pitchClass)
{
  return !isBlackPitchClass(pitchClass);
}

bool isValidRange(const PitchRange& range)
{
  return range.minPitch <= range.maxPitch;
}

double positiveOrDefault(const double value, const double fallback)
{
  return std::isfinite(value) && value > 0.0 ? value : fallback;
}

KeyboardLayoutConfig sanitizedConfig(KeyboardLayoutConfig config)
{
  config.whiteKeyWidth = positiveOrDefault(config.whiteKeyWidth, 1.0);
  config.whiteKeyHeight = positiveOrDefault(config.whiteKeyHeight, 1.0);
  config.blackKeyWidth = positiveOrDefault(config.blackKeyWidth, 0.6);
  config.blackKeyHeight = positiveOrDefault(config.blackKeyHeight, 0.62);
  if (!std::isfinite(config.whiteKeyGap) || config.whiteKeyGap < 0.0) {
    config.whiteKeyGap = 0.015;
  }
  config.whiteKeyGap =
    std::min(config.whiteKeyGap, std::max(0.0, config.whiteKeyWidth - kMinimumPositiveWidth));
  return config;
}

Rect invalidRect()
{
  return {};
}

bool hasPositiveArea(const Rect& rect)
{
  return rect.width > 0.0 && rect.height > 0.0;
}

} // namespace

KeyboardGeometry::KeyboardGeometry(const KeyboardLayoutConfig& config)
    : m_config(sanitizedConfig(config))
{
  if (!isValidRange(m_config.pitchRange)) {
    return;
  }

  std::unordered_map<int, double> whiteKeyXByPitch;
  for (auto pitch = m_config.pitchRange.minPitch; pitch <= m_config.pitchRange.maxPitch; ++pitch) {
    if (!isWhiteKey(pitch)) {
      continue;
    }

    const auto x = static_cast<double>(m_whiteKeyCount) * m_config.whiteKeyWidth;
    const Rect keyRect{
      .x = x,
      .y = -m_config.whiteKeyHeight,
      .width = m_config.whiteKeyWidth,
      .height = m_config.whiteKeyHeight,
    };

    const auto noteGap =
      std::min(m_config.whiteKeyGap, std::max(0.0, m_config.whiteKeyWidth - kMinimumPositiveWidth));
    const Rect noteRect{
      .x = x + noteGap * 0.5,
      .y = 0.0,
      .width = std::max(kMinimumPositiveWidth, m_config.whiteKeyWidth - noteGap),
      .height = 0.0,
    };

    m_keyRects.emplace(pitch, keyRect);
    m_noteRects.emplace(pitch, noteRect);
    whiteKeyXByPitch.emplace(pitch, x);
    ++m_whiteKeyCount;
  }

  m_width = static_cast<double>(m_whiteKeyCount) * m_config.whiteKeyWidth;

  // Black keys are positioned from the surrounding white keys, so a black key at a clipped
  // pitch-range edge is omitted unless both neighboring white-key anchors exist.
  for (auto pitch = m_config.pitchRange.minPitch; pitch <= m_config.pitchRange.maxPitch; ++pitch) {
    if (!isBlackKey(pitch)) {
      continue;
    }

    const auto previousWhite = pitch - 1;
    const auto nextWhite = pitch + 1;
    const auto previousWhiteIt = whiteKeyXByPitch.find(previousWhite);
    if (previousWhiteIt == whiteKeyXByPitch.end() || !whiteKeyXByPitch.contains(nextWhite)) {
      continue;
    }

    const auto x =
      previousWhiteIt->second + m_config.whiteKeyWidth - (m_config.blackKeyWidth * 0.5);
    const Rect keyRect{
      .x = x,
      .y = -m_config.blackKeyHeight,
      .width = m_config.blackKeyWidth,
      .height = m_config.blackKeyHeight,
    };
    const Rect noteRect{
      .x = x,
      .y = 0.0,
      .width = m_config.blackKeyWidth,
      .height = 0.0,
    };

    m_keyRects.emplace(pitch, keyRect);
    m_noteRects.emplace(pitch, noteRect);
  }
}

Rect KeyboardGeometry::keyRectForPitch(const int pitch) const
{
  const auto it = m_keyRects.find(pitch);
  return it == m_keyRects.end() ? invalidRect() : it->second;
}

Rect KeyboardGeometry::noteRectForPitch(const int pitch) const
{
  const auto it = m_noteRects.find(pitch);
  return it == m_noteRects.end() ? invalidRect() : it->second;
}

bool KeyboardGeometry::containsPitch(const int pitch) const
{
  return pitch >= m_config.pitchRange.minPitch && pitch <= m_config.pitchRange.maxPitch &&
         hasPositiveArea(keyRectForPitch(pitch));
}

bool KeyboardGeometry::isBlackKey(const int pitch)
{
  return isBlackPitchClass(pitchClass(pitch));
}

bool KeyboardGeometry::isWhiteKey(const int pitch)
{
  return isWhitePitchClass(pitchClass(pitch));
}

int KeyboardGeometry::whiteKeyCount() const
{
  return m_whiteKeyCount;
}

double KeyboardGeometry::width() const
{
  return m_width;
}

double KeyboardGeometry::height() const
{
  return m_config.whiteKeyHeight;
}

const KeyboardLayoutConfig& KeyboardGeometry::config() const
{
  return m_config;
}

KeyboardLayoutConfig keyboardLayoutConfigFromSettings(const KeyboardSettings& settings,
                                                      const PitchRange& pitchRange)
{
  const auto sanitizedSettings = sanitizeKeyboardSettings(settings);
  return KeyboardLayoutConfig{
    .pitchRange = pitchRange,
    .whiteKeyWidth = sanitizedSettings.whiteKeyWidth,
    .whiteKeyHeight = sanitizedSettings.whiteKeyHeight,
    .blackKeyWidth = sanitizedSettings.blackKeyWidth,
    .blackKeyHeight = sanitizedSettings.blackKeyHeight,
    .whiteKeyGap = sanitizedSettings.whiteKeyGap,
  };
}
