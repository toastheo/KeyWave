#include "audio/FluidSynthAudioCallback.hpp"

#include <array>
#include <atomic>
#include <cstddef>
#include <fluidsynth.h> // IWYU pragma: keep -- defines the public API/export macros
#include <fluidsynth/audio.h>
#include <fluidsynth/misc.h>
#include <span>
#include <type_traits>

// FluidSynth requires its C callback type; adapt buffers to C++ views inside.
static_assert(std::is_same_v<decltype(&renderFluidSynthAudio), fluid_audio_func_t>);

int renderFluidSynthAudio(void* const data,
                          const int frameCount,
                          const int effectBufferCount,
                          float** effectBuffers,
                          const int outputBufferCount,
                          float** outputBuffers)
{
  if (data == nullptr) {
    return FLUID_FAILED;
  }

  auto& state = *static_cast<FluidSynthAudioRenderState*>(data);
  if (state.playbackPaused.load(std::memory_order_relaxed)) {
    // FluidSynth's audio driver provides pre-zeroed output buffers. Skipping
    // processing outputs silence while preserving voices, envelopes and effects.
    return FLUID_OK;
  }

  if (state.synth == nullptr || state.process == nullptr) {
    return FLUID_FAILED;
  }

  if (effectBufferCount == 0) {
    // Most audio drivers provide no dedicated effect buffers. FluidSynth supports
    // aliasing them with the stereo output so effects are mixed into that output.

    if (outputBufferCount < 2 || outputBuffers == nullptr) {
      return FLUID_FAILED;
    }

    const std::span<float*> outputs{outputBuffers, static_cast<std::size_t>(outputBufferCount)};
    std::array<float*, 4> mixedEffectBuffers{
      outputs[0],
      outputs[1],
      outputs[0],
      outputs[1],
    };
    return state.process(state.synth,
                         frameCount,
                         static_cast<int>(mixedEffectBuffers.size()),
                         mixedEffectBuffers.data(),
                         outputBufferCount,
                         outputs.data());
  }

  return state.process(
    state.synth, frameCount, effectBufferCount, effectBuffers, outputBufferCount, outputBuffers);
}
