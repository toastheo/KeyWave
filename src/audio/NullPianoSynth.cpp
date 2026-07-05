#include "audio/NullPianoSynth.hpp"

void NullPianoSynth::noteOn(const int pitch, const int velocity)
{
  (void)pitch;
  (void)velocity;
}

void NullPianoSynth::noteOff(const int pitch)
{
  (void)pitch;
}

void NullPianoSynth::allNotesOff() {}
