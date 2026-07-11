#include "audio/NullPianoSynth.hpp"

#include "audio/PianoSynth.hpp"

void NullPianoSynth::noteOn(const PianoNote note)
{
  (void)note;
}

void NullPianoSynth::noteOff(const int pitch)
{
  (void)pitch;
}

void NullPianoSynth::setSustainPedal(const SustainPedalState state)
{
  (void)state;
}

void NullPianoSynth::allNotesOff() {}
