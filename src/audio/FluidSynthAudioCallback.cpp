#include "audio/FluidSynthAudioCallback.hpp"

#include <fluidsynth.h>

int renderFluidSynthAudio(void* const data,
                          const int frameCount,
                          const int effectBufferCount,
                          float* effectBuffers[],
                          const int outputBufferCount,
                          float* outputBuffers[])
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

    constexpr int mixedEffectBufferCount = 4;
    float* mixedEffectBuffers[mixedEffectBufferCount]{
      outputBuffers[0],
      outputBuffers[1],
      outputBuffers[0],
      outputBuffers[1],
    };
    return state.process(state.synth,
                         frameCount,
                         mixedEffectBufferCount,
                         mixedEffectBuffers,
                         outputBufferCount,
                         outputBuffers);
  }

  return state.process(
    state.synth, frameCount, effectBufferCount, effectBuffers, outputBufferCount, outputBuffers);
}
