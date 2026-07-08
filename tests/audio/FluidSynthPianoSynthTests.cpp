#include <catch2/catch_test_macros.hpp>
#include <filesystem>

#include "audio/FluidSynthPianoSynth.hpp"
#include "diagnostics/RecordingDiagnosticSink.hpp"

namespace {

TEST_CASE("FluidSynthPianoSynth reports missing SoundFont and stays safely silent", "[audio]")
{
  RecordingDiagnosticSink diagnostics;
  const auto missingSoundFont =
    std::filesystem::path{KEYWAVE_SOURCE_DIR} / "assets" / "soundfonts" / "missing.sf2";

  FluidSynthPianoSynth synth(missingSoundFont, diagnostics);

  synth.noteOn(PianoNote{.pitch = 60, .velocity = 90});
  synth.noteOff(60);
  synth.allNotesOff();

  REQUIRE(diagnostics.messages.size() == 1);
  CHECK(diagnostics.messages[0].severity == DiagnosticSeverity::Error);
  CHECK(diagnostics.messages[0].message.find("FluidSynth SoundFont not found:") == 0);
}

} // namespace