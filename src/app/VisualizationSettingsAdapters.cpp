#include "app/VisualizationSettingsAdapters.hpp"

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

KeyboardRenderStyle keyboardRenderStyleFromSettings(const KeyboardSettings& settings)
{
  const auto sanitizedSettings = sanitizeKeyboardSettings(settings);
  return KeyboardRenderStyle{
    .whiteKeyColor = sanitizedSettings.whiteKeyColor,
    .blackKeyColor = sanitizedSettings.blackKeyColor,
    .activeWhiteKeyColor = sanitizedSettings.activeWhiteKeyColor,
    .activeBlackKeyColor = sanitizedSettings.activeBlackKeyColor,
    .whiteKeySeparatorColor = sanitizedSettings.whiteKeySeparatorColor,
    .hitLineColor = sanitizedSettings.hitLineColor,
    .separatorThicknessPixels = sanitizedSettings.separatorWidth,
    .hitLineHeight = sanitizedSettings.hitLineHeight,
    .includeSeparators = sanitizedSettings.includeSeparators,
    .includeHitLine = sanitizedSettings.includeHitLine,
  };
}

FallingNotesLayoutStyle fallingNotesLayoutStyleFromSettings(const FallingNotesSettings& settings)
{
  const auto sanitizedSettings = sanitizeFallingNotesSettings(settings);
  return FallingNotesLayoutStyle{
    .noteHorizontalInset = sanitizedSettings.noteHorizontalInset,
    .blackNoteWidthScale = sanitizedSettings.blackNoteWidthScale,
    .whiteNoteWidthScale = sanitizedSettings.whiteNoteWidthScale,
  };
}

FallingNotesRenderStyle fallingNotesRenderStyleFromSettings(const FallingNotesSettings& settings)
{
  const auto sanitizedSettings = sanitizeFallingNotesSettings(settings);
  return FallingNotesRenderStyle{
    .noteColor = sanitizedSettings.noteColor,
    .activeNoteColor = sanitizedSettings.activeNoteColor,
    .outlineColor = sanitizedSettings.outlineColor,
    .outlineThicknessPixels = sanitizedSettings.outlineThicknessPixels,
    .includeOutline = sanitizedSettings.includeOutline,
  };
}

PianoRollSceneConfig pianoRollSceneConfigFromSettings(
  const FallingNotesSettings& fallingNotesSettings, const KeyboardSettings& keyboardSettings)
{
  const auto sanitizedFallingNotesSettings = sanitizeFallingNotesSettings(fallingNotesSettings);
  return PianoRollSceneConfig{
    .pitchRange = sanitizedFallingNotesSettings.pitchRange,
    .lookAheadSeconds = sanitizedFallingNotesSettings.lookAheadSeconds,
    .visiblePastSeconds = sanitizedFallingNotesSettings.visiblePastSeconds,
    .keyboardLayout =
      keyboardLayoutConfigFromSettings(keyboardSettings, sanitizedFallingNotesSettings.pitchRange),
    .fallingNotesLayout = fallingNotesLayoutStyleFromSettings(sanitizedFallingNotesSettings),
    .fallingNotesStyle = fallingNotesRenderStyleFromSettings(sanitizedFallingNotesSettings),
    .keyboardStyle = keyboardRenderStyleFromSettings(keyboardSettings),
  };
}
