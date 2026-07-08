#include "audio/FluidSynthPianoSynth.hpp"

#include <algorithm>
#include <filesystem>
#include <fluidsynth.h>
#include <memory>
#include <string>
#include <system_error>
#include <utility>

#include "diagnostics/Diagnostics.hpp"

namespace {

constexpr int pianoMidiChannel = 0;
constexpr int midiPitchMin = 0;
constexpr int midiPitchMax = 127;
constexpr int midiVelocityMin = 0;
constexpr int midiVelocityMax = 127;

std::string pathString(const std::filesystem::path& path)
{
  return path.string();
}

} // namespace

struct FluidSynthPianoSynth::Handles
{
  ~Handles()
  {
    if (audioDriver != nullptr) {
      delete_fluid_audio_driver(audioDriver);
    }
    if (synth != nullptr) {
      delete_fluid_synth(synth);
    }
    if (settings != nullptr) {
      delete_fluid_settings(settings);
    }
  }

  fluid_settings_t* settings = nullptr;
  fluid_synth_t* synth = nullptr;
  fluid_audio_driver_t* audioDriver = nullptr;
};

FluidSynthPianoSynth::FluidSynthPianoSynth(std::filesystem::path soundFontPath,
                                           DiagnosticSink& diagnostics)
    : m_diagnostics(diagnostics)
{
  const auto soundFontString = pathString(soundFontPath);
  std::error_code existsError;
  if (!std::filesystem::exists(soundFontPath, existsError)) {
    const auto messagePrefix = existsError ? "FluidSynth SoundFont path could not be checked: "
                                           : "FluidSynth SoundFont not found: ";
    reportError(m_diagnostics, messagePrefix + soundFontString);
    return;
  }

  auto handles = std::make_unique<Handles>();
  handles->settings = new_fluid_settings();
  if (handles->settings == nullptr) {
    reportError(m_diagnostics, "FluidSynth initialization failed: Settings could not be created.");
    return;
  }

  handles->synth = new_fluid_synth(handles->settings);
  if (handles->synth == nullptr) {
    reportError(m_diagnostics, "FluidSynth initialization failed: Synth could not be created.");
    return;
  }

  const auto soundFontId = fluid_synth_sfload(handles->synth, soundFontString.c_str(), 1);
  if (soundFontId == FLUID_FAILED) {
    reportError(m_diagnostics, "FluidSynth could not load SoundFont: " + soundFontString);
    return;
  }

  handles->audioDriver = new_fluid_audio_driver(handles->settings, handles->synth);
  if (handles->audioDriver == nullptr) {
    reportError(m_diagnostics,
                "FluidSynth initialization failed: Audio driver could not be created.");
    return;
  }

  m_handles = std::move(handles);
}

FluidSynthPianoSynth::~FluidSynthPianoSynth() = default;

void FluidSynthPianoSynth::noteOn(const PianoNote note)
{
  if (m_handles == nullptr) {
    return;
  }

  const auto pitch = std::clamp(note.pitch, midiPitchMin, midiPitchMax);
  const auto velocity = std::clamp(note.velocity, midiVelocityMin, midiVelocityMax);
  if (fluid_synth_noteon(m_handles->synth, pianoMidiChannel, pitch, velocity) == FLUID_FAILED) {
    reportWarning(m_diagnostics, "FluidSynth note-on failed.");
  }
}

void FluidSynthPianoSynth::noteOff(const int pitch)
{
  if (m_handles == nullptr) {
    return;
  }

  const auto clampedPitch = std::clamp(pitch, midiPitchMin, midiPitchMax);
  if (fluid_synth_noteoff(m_handles->synth, pianoMidiChannel, clampedPitch) == FLUID_FAILED) {
    reportWarning(m_diagnostics, "FluidSynth note-off failed.");
  }
}

void FluidSynthPianoSynth::allNotesOff()
{
  if (m_handles == nullptr) {
    return;
  }

  if (fluid_synth_all_notes_off(m_handles->synth, pianoMidiChannel) == FLUID_FAILED) {
    reportWarning(m_diagnostics, "FluidSynth all-notes-off failed.");
  }
}

bool FluidSynthPianoSynth::available() const
{
  return m_handles != nullptr;
}
