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

struct PitchRange
{
  int minPitch = 0;
  int maxPitch = 0;
};
