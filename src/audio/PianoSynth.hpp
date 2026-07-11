#pragma once

struct PianoNote
{
  int pitch = 0;
  int velocity = 0;
};

enum class SustainPedalState
{
  Up,
  Down,
};

class PianoSynth
{
public:
  virtual ~PianoSynth() = default;

  virtual void noteOn(PianoNote note) = 0;
  virtual void noteOff(int pitch) = 0;
  virtual void setSustainPedal(SustainPedalState state) = 0;
  virtual void allNotesOff() = 0;
};
