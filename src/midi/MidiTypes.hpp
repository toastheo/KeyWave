#pragma once

struct Note
{
  int pitch = 0;
  int velocity = 0;
  int channel = 0;
  int track = 0;
  double startSeconds = 0.0;
  double durationSeconds = 0.0;
};

inline constexpr double defaultMidiBpm = 120.0;

struct TempoEvent
{
  double timeSeconds = 0.0;
  double bpm = defaultMidiBpm;
};

struct SustainPedalEvent
{
  double timeSeconds = 0.0;
  bool pressed = false;
  int channel = 0;
  int track = 0;
};
