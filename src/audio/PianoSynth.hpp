#pragma once

class PianoSynth
{
public:
  virtual ~PianoSynth() = default;

  virtual void noteOn(int pitch, int velocity) = 0;
  virtual void noteOff(int pitch) = 0;
  virtual void allNotesOff() = 0;
};
