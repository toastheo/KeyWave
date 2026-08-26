#pragma once

#include <atomic>

using FluidSynthProcessCallback = int (*)(void* synth,
                                          int frameCount,
                                          int effectBufferCount,
                                          float* effectBuffers[],
                                          int outputBufferCount,
                                          float* outputBuffers[]);

struct FluidSynthAudioRenderState
{
  void* synth = nullptr;
  FluidSynthProcessCallback process = nullptr;
  // Written by the transport thread and read by the audio thread.
  std::atomic<bool> playbackPaused{false};
};

int renderFluidSynthAudio(void* data,
                          int frameCount,
                          int effectBufferCount,
                          float* effectBuffers[],
                          int outputBufferCount,
                          float* outputBuffers[]);
