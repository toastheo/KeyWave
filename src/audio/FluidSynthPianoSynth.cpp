#include "audio/FluidSynthPianoSynth.hpp"

#include <algorithm>
#include <filesystem>
#include <fluidsynth.h>
#include <memory>
#include <string>
#include <system_error>
#include <utility>

#include "audio/PianoSynth.hpp"
#include "diagnostics/Diagnostics.hpp"

namespace {

constexpr int pianoMidiChannel = 0;
constexpr int midiPitchMin = 0;
constexpr int midiPitchMax = 127;
constexpr int midiVelocityMin = 0;
constexpr int midiVelocityMax = 127;
constexpr int sustainPedalController = 64;
constexpr int sustainPedalOffValue = 0;
constexpr int sustainPedalOnValue = 127;
constexpr int audioPeriodSize = 256;
constexpr int audioPeriods = 8;

std::string pathString(const std::filesystem::path& path)
{
  return path.string();
}

void setFluidSynthIntSetting(fluid_settings_t& settings,
                             DiagnosticSink& diagnostics,
                             const char* const name,
                             const int value)
{
  if (fluid_settings_setint(&settings, name, value) == FLUID_FAILED) {
    reportWarning(diagnostics, std::string{"FluidSynth setting ignored: "} + name);
  }
}

void configureFluidSynthSettings(fluid_settings_t& settings, DiagnosticSink& diagnostics)
{
  // We normalize the driver buffer shape. Some platforms default to tiny periods, which
  // are good for live input but can crackle on some audio backends.
  setFluidSynthIntSetting(settings, diagnostics, "audio.period-size", audioPeriodSize);
  setFluidSynthIntSetting(settings, diagnostics, "audio.periods", audioPeriods);
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

FluidSynthPianoSynth::FluidSynthPianoSynth(const std::filesystem::path& soundFontPath,
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
  configureFluidSynthSettings(*handles->settings, m_diagnostics);

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

void FluidSynthPianoSynth::setSustainPedal(const SustainPedalState state)
{
  if (m_handles == nullptr) {
    return;
  }

  const auto value = state == SustainPedalState::Down ? sustainPedalOnValue : sustainPedalOffValue;
  if (fluid_synth_cc(m_handles->synth, pianoMidiChannel, sustainPedalController, value) ==
      FLUID_FAILED) {
    reportWarning(m_diagnostics, "FluidSynth sustain pedal control failed.");
  }
}

void FluidSynthPianoSynth::allNotesOff()
{
  if (m_handles == nullptr) {
    return;
  }

  if (fluid_synth_cc(
        m_handles->synth, pianoMidiChannel, sustainPedalController, sustainPedalOffValue) ==
      FLUID_FAILED) {
    reportWarning(m_diagnostics, "FluidSynth sustain pedal control failed.");
  }

  if (fluid_synth_all_notes_off(m_handles->synth, pianoMidiChannel) == FLUID_FAILED) {
    reportWarning(m_diagnostics, "FluidSynth all-notes-off failed.");
  }

  if (fluid_synth_all_sounds_off(m_handles->synth, pianoMidiChannel) == FLUID_FAILED) {
    reportWarning(m_diagnostics, "FluidSynth all-sounds-off failed.");
  }
}

bool FluidSynthPianoSynth::available() const
{
  return m_handles != nullptr;
}
