#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

#include "core/CoreTypes.hpp"

enum class VideoEncoderState : std::uint8_t
{
  Idle,
  Starting,
  Running,
  Finalizing,
  Completed,
  Failed,
  Cancelled
};

struct VideoEncoderConfig
{
  FramebufferSize size{.width = 1920, .height = 1080};
  int fps = 30;
  std::uint64_t frameCount = 0;
};

struct VideoEncoderStatus
{
  VideoEncoderState state = VideoEncoderState::Idle;
  std::uint64_t framesWritten = 0;
  std::string error;
};

// Frames are tightly packed RGBA8, top row first. All caller methods run on
// the UI thread. start/submit/finish/cancel never perform pipe I/O or wait for
// a child process. Submission moves ownership only when accepted. Completion
// means a successfully finalized MP4 was published without replacing a file.
class VideoEncoder
{
public:
  virtual ~VideoEncoder() = default;
  virtual void start(VideoEncoderConfig config, const std::filesystem::path& destination) = 0;
  [[nodiscard]] virtual VideoEncoderStatus status() const = 0;
  [[nodiscard]] virtual bool canAcceptFrame() const = 0;
  virtual bool trySubmit(std::vector<std::uint8_t>&& rgba) = 0;
  virtual void finish() = 0;
  virtual void cancel() = 0;
};
