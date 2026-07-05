#include "audio/NullPianoSynth.hpp"

void NullPianoSynth::noteOn(const PianoNote note)
{
  (void)note;
}

void NullPianoSynth::noteOff(const int pitch)
{
  (void)pitch;
}

void NullPianoSynth::allNotesOff() {}
