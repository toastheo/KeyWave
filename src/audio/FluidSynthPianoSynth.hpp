#pragma once

#include <filesystem>
#include <memory>

#include "audio/PianoSynth.hpp"
#include "diagnostics/Diagnostics.hpp"

class FluidSynthPianoSynth final : public PianoSynth
{
public:
  explicit FluidSynthPianoSynth(const std::filesystem::path& soundFontPath,
                                DiagnosticSink& diagnostics = nullDiagnosticSink());
  ~FluidSynthPianoSynth() override;

  FluidSynthPianoSynth(const FluidSynthPianoSynth&) = delete;
  FluidSynthPianoSynth& operator=(const FluidSynthPianoSynth&) = delete;
  FluidSynthPianoSynth(FluidSynthPianoSynth&&) = delete;
  FluidSynthPianoSynth& operator=(FluidSynthPianoSynth&&) = delete;

  void noteOn(PianoNote note) override;
  void noteOff(int pitch) override;
  void allNotesOff() override;

  [[nodiscard]] bool available() const;

private:
  struct Handles;

  DiagnosticSink& m_diagnostics;
  std::unique_ptr<Handles> m_handles;
};
