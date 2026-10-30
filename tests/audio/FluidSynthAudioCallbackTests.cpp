#include <array>
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

// This test callback must match the FluidSynth C API parameter order and types.
// NOLINTBEGIN(bugprone-easily-swappable-parameters)
int recordProcess(void* const synth,
                  const int frameCount,
                  const int effectBufferCount,
                  float** effectBuffers,
                  const int outputBufferCount,
                  float** outputBuffers)
// NOLINTEND(bugprone-easily-swappable-parameters)
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

  std::array<float, 4> left{};
  std::array<float, 4> right{};
  std::array outputs{left.data(), right.data()};

  CHECK(renderFluidSynthAudio(&state, 4, 0, nullptr, 2, outputs.data()) == 0);
  CHECK(call.count == 0);
}

TEST_CASE("FluidSynth audio callback mixes effects into stereo output buffers",
          "[audio][fluidsynth]")
{
  ProcessCall call;
  FluidSynthAudioRenderState state;
  state.synth = &call;
  state.process = &recordProcess;

  std::array<float, 4> left{};
  std::array<float, 4> right{};
  std::array outputs{left.data(), right.data()};

  CHECK(renderFluidSynthAudio(&state, 4, 0, nullptr, 2, outputs.data()) == call.result);
  CHECK(call.count == 1);
  CHECK(call.frameCount == 4);
  CHECK(call.effectBuffers ==
        std::vector<float*>{left.data(), right.data(), left.data(), right.data()});
  CHECK(call.outputBuffers == std::vector<float*>{left.data(), right.data()});
}

TEST_CASE("FluidSynth audio callback forwards dedicated effects buffers", "[audio][fluidsynth]")
{
  ProcessCall call;
  FluidSynthAudioRenderState state;
  state.synth = &call;
  state.process = &recordProcess;

  std::array<float, 4> reverb{};
  std::array<float, 4> chorus{};
  std::array<float, 4> left{};
  std::array<float, 4> right{};
  std::array effects{reverb.data(), chorus.data()};
  std::array outputs{left.data(), right.data()};

  CHECK(renderFluidSynthAudio(&state, 4, 2, effects.data(), 2, outputs.data()) == call.result);
  CHECK(call.count == 1);
  CHECK(call.effectBuffers == std::vector<float*>{reverb.data(), chorus.data()});
  CHECK(call.outputBuffers == std::vector<float*>{left.data(), right.data()});
}

} // namespace
