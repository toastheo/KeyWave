#include <catch2/catch_test_macros.hpp>

#include "audio/NullPianoSynth.hpp"

namespace {

TEST_CASE("NullPianoSynth implements the piano synth interface as a no-op", "[audio]")
{
  NullPianoSynth nullSynth;
  PianoSynth& synth = nullSynth;

  synth.noteOn(60, 96);
  synth.noteOff(60);
  synth.allNotesOff();

  SUCCEED("NullPianoSynth accepts piano synth commands without observable side effects");
}

} // namespace
