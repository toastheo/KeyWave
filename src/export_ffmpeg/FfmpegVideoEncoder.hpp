#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <memory>
#include <vector>

#include "export/VideoEncoder.hpp"

class FfmpegVideoEncoder final : public VideoEncoder
{
public:
  // Empty path discovers ffmpeg beside the application, then on PATH.
  // Publication runs on the worker and must refuse an existing destination.
  using PublishVideo = std::function<void(const std::filesystem::path& temporary,
                                          const std::filesystem::path& destination)>;
  explicit FfmpegVideoEncoder(std::filesystem::path executable = {}, PublishVideo publisher = {});
  ~FfmpegVideoEncoder() override;
  void start(VideoEncoderConfig config, const std::filesystem::path& destination) override;
  [[nodiscard]] VideoEncoderStatus status() const override;
  [[nodiscard]] bool canAcceptFrame() const override;
  bool trySubmit(std::vector<std::uint8_t>&& rgba) override;
  void finish() override;
  void cancel() override;

  static constexpr std::size_t queueCapacity = 3;

private:
  struct Impl;
  std::unique_ptr<Impl> m_impl;
};