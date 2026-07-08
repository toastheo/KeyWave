#pragma once

struct DoubleSettingRange
{
  double minimum = 0.0;
  double maximum = 0.0;
};

struct DoubleSettingMinimum
{
  double minimum = 0.0;
};

struct IntSettingRange
{
  int minimum = 0;
  int maximum = 0;
};

struct FallingNotesSettingsConstraints
{
  IntSettingRange pitchRange{.minimum = 0, .maximum = 127};
  DoubleSettingRange lookAheadSeconds{.minimum = 1.0, .maximum = 30.0};
  DoubleSettingRange visiblePastSeconds{.minimum = 0.0, .maximum = 5.0};
  DoubleSettingRange noteHorizontalInset{.minimum = 0.0, .maximum = 0.5};
  DoubleSettingRange noteWidthScale{.minimum = 0.05, .maximum = 1.0};
  DoubleSettingRange outlineThicknessPixels{.minimum = 0.0, .maximum = 8.0};
  DoubleSettingRange cornerRadiusPixels{.minimum = 0.0, .maximum = 24.0};
};

struct KeyboardSettingsConstraints
{
  double minimumPositiveValue = 0.000001;
  DoubleSettingRange whiteKeyWidth{.minimum = 0.1, .maximum = 3.0};
  DoubleSettingRange whiteKeyHeight{.minimum = 0.2, .maximum = 6.0};

  // The maximum size of the black keys is calculated dynamically based on
  // the size of the white keys, so we'll just define a minimum here.
  DoubleSettingMinimum blackKeyWidth{.minimum = 0.05};
  DoubleSettingMinimum blackKeyHeight{.minimum = 0.1};
  DoubleSettingRange separatorWidth{.minimum = 0.0, .maximum = 8.0};
  DoubleSettingRange hitLineHeight{.minimum = 0.005, .maximum = 0.25};
};

struct AppSettingsConstraints
{
  FallingNotesSettingsConstraints fallingNotes;
  KeyboardSettingsConstraints keyboard;
};

[[nodiscard]] const AppSettingsConstraints& appSettingsConstraints();
