#include <catch2/catch_test_macros.hpp>
#include <vector>

#include "audio/FluidSynthAudioCallback.hpp"

namespace {

struct ProcessCall
{
  int result = 37;
  int count = 0;
  int frameCount = 0;
  int outputBufferCount = 0;
  std::vector<float*> effectBuffers;
  std::vector<float*> outputBuffers;
};

int recordProcess(void* const synth,
                  const int frameCount,
                  const int effectBufferCount,
                  float* effectBuffers[],
                  const int outputBufferCount,
                  float* outputBuffers[])
{
  auto& call = *static_cast<ProcessCall*>(synth);
  ++call.count;
  call.frameCount = frameCount;
  call.outputBufferCount = outputBufferCount;
  call.effectBuffers.assign(effectBuffers, effectBuffers + effectBufferCount);
  call.outputBuffers.assign(outputBuffers, outputBuffers + outputBufferCount);
  return call.result;
}

TEST_CASE("FluidSynth audio callback leaves silence untouched while playback is paused",
          "[audio][fluidsynth]")
{
  ProcessCall call;
  FluidSynthAudioRenderState state;
  state.synth = &call;
  state.process = &recordProcess;
  state.playbackPaused.store(true);

  float left[4]{};
  float right[4]{};
  float* outputs[]{left, right};

  CHECK(renderFluidSynthAudio(&state, 4, 0, nullptr, 2, outputs) == 0);
  CHECK(call.count == 0);
}

TEST_CASE("FluidSynth audio callback mixes effects into stereo output buffers",
          "[audio][fluidsynth]")
{
  ProcessCall call;
  FluidSynthAudioRenderState state;
  state.synth = &call;
  state.process = &recordProcess;

  float left[4]{};
  float right[4]{};
  float* outputs[]{left, right};

  CHECK(renderFluidSynthAudio(&state, 4, 0, nullptr, 2, outputs) == call.result);
  CHECK(call.count == 1);
  CHECK(call.frameCount == 4);
  CHECK(call.effectBuffers == std::vector<float*>{left, right, left, right});
  CHECK(call.outputBuffers == std::vector<float*>{left, right});
}

TEST_CASE("FluidSynth audio callback forwards dedicated effects buffers", "[audio][fluidsynth]")
{
  ProcessCall call;
  FluidSynthAudioRenderState state;
  state.synth = &call;
  state.process = &recordProcess;

  float reverb[4]{};
  float chorus[4]{};
  float left[4]{};
  float right[4]{};
  float* effects[]{reverb, chorus};
  float* outputs[]{left, right};

  CHECK(renderFluidSynthAudio(&state, 4, 2, effects, 2, outputs) == call.result);
  CHECK(call.count == 1);
  CHECK(call.effectBuffers == std::vector<float*>{reverb, chorus});
  CHECK(call.outputBuffers == std::vector<float*>{left, right});
}

} // namespace
