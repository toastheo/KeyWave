#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "core/CoreTypes.hpp"

// RGBA8 framebuffer channels, tightly packed rows starting at the top left(!).
// No color-space conversion is performed. strideBytes is always width * 4.
struct OffscreenPixelBuffer
{
  FramebufferSize size;
  std::size_t strideBytes = 0;
  std::vector<std::uint8_t> pixels;
};

// All operations, including destruction, require the owning OpenGL 3.3 context
// to be current on its context thread, with GLAD already initialized.
// Enter with a clean OpenGL error state; allocation/readback report GL errors
// as std::runtime_error, and invalid dimensions as std::invalid_argument.
class OffscreenRenderTarget final
{
public:
  explicit OffscreenRenderTarget(FramebufferSize size);
  ~OffscreenRenderTarget();

  OffscreenRenderTarget(const OffscreenRenderTarget&) = delete;
  OffscreenRenderTarget& operator=(const OffscreenRenderTarget&) = delete;
  OffscreenRenderTarget(OffscreenRenderTarget&&) = delete;
  OffscreenRenderTarget& operator=(OffscreenRenderTarget&&) = delete;

  [[nodiscard]] FramebufferSize size() const;
  [[nodiscard]] unsigned int framebuffer() const;
  [[nodiscard]] OffscreenPixelBuffer readPixels() const;

private:
  FramebufferSize m_size;
  unsigned int m_framebuffer = 0;
  unsigned int m_colorTexture = 0;
};
