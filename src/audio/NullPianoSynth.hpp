#pragma once

#include "audio/PianoSynth.hpp"

class NullPianoSynth final : public PianoSynth
{
public:
  void noteOn(int pitch, int velocity) override;
  void noteOff(int pitch) override;
  void allNotesOff() override;
};
