#include "export_ffmpeg/FfmpegVideoEncoder.hpp"

#include <stdexcept>

struct FfmpegVideoEncoder::Impl {};
FfmpegVideoEncoder::FfmpegVideoEncoder(std::filesystem::path) : m_impl(std::make_unique<Impl>()) {}
FfmpegVideoEncoder::~FfmpegVideoEncoder() = default;
void FfmpegVideoEncoder::start(VideoEncoderConfig, const std::filesystem::path&) {}
VideoEncoderStatus FfmpegVideoEncoder::status() const { return {}; }
bool FfmpegVideoEncoder::canAcceptFrame() const { return false; }
bool FfmpegVideoEncoder::trySubmit(std::vector<std::uint8_t>&&) { return false; }
void FfmpegVideoEncoder::finish() {}
void FfmpegVideoEncoder::cancel() {}
