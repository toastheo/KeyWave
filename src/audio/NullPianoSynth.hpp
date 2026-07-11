#pragma once

#include "audio/PianoSynth.hpp"

class NullPianoSynth final : public PianoSynth
{
public:
  void noteOn(PianoNote note) override;
  void noteOff(int pitch) override;
  void setSustainPedal(SustainPedalState state) override;
  void allNotesOff() override;
};
