#include "render_opengl/OffscreenRenderTarget.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <glad/gl.h>
#include <ios>
#include <limits>
#include <span>
#include <sstream>
#include <stdexcept>

#include "core/CoreTypes.hpp"
#include "render/RendererView.hpp"

namespace {

// For saving (and restoring) the current OpenGL state
struct AllocationState
{
  GLint drawFramebuffer = 0;
  GLint readFramebuffer = 0;
  GLint texture = 0;
  GLint unpackBuffer = 0;

  AllocationState()
  {
    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &drawFramebuffer);
    glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &readFramebuffer);
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &texture);
    glGetIntegerv(GL_PIXEL_UNPACK_BUFFER_BINDING, &unpackBuffer);
  }

  ~AllocationState()
  {
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, static_cast<GLuint>(drawFramebuffer));
    glBindFramebuffer(GL_READ_FRAMEBUFFER, static_cast<GLuint>(readFramebuffer));
    glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(texture));
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, static_cast<GLuint>(unpackBuffer));
  }
};

// Same trick as above but for the pixel pack state
struct ReadbackState
{
  GLint framebuffer = 0;
  GLint packBuffer = 0;
  GLint alignment = 0;
  GLint rowLength = 0;
  GLint skipRows = 0;
  GLint skipPixels = 0;

  ReadbackState()
  {
    glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &framebuffer);
    glGetIntegerv(GL_PIXEL_PACK_BUFFER_BINDING, &packBuffer);
    glGetIntegerv(GL_PACK_ALIGNMENT, &alignment);
    glGetIntegerv(GL_PACK_ROW_LENGTH, &rowLength);
    glGetIntegerv(GL_PACK_SKIP_ROWS, &skipRows);
    glGetIntegerv(GL_PACK_SKIP_PIXELS, &skipPixels);
  }

  ~ReadbackState()
  {
    glBindFramebuffer(GL_READ_FRAMEBUFFER, static_cast<GLuint>(framebuffer));
    glBindBuffer(GL_PIXEL_PACK_BUFFER, static_cast<GLuint>(packBuffer));
    glPixelStorei(GL_PACK_ALIGNMENT, alignment);
    glPixelStorei(GL_PACK_ROW_LENGTH, rowLength);
    glPixelStorei(GL_PACK_SKIP_ROWS, skipRows);
    glPixelStorei(GL_PACK_SKIP_PIXELS, skipPixels);
  }
};

} // namespace

OffscreenRenderTarget::OffscreenRenderTarget(const FramebufferSize size)
    : m_size(size)
{
  if (!isValid(size)) {
    throw std::invalid_argument("Offscreen target dimensions must be positive.");
  }
  GLint maxTexture = 0;
  GLint maxRenderbuffer = 0;
  std::array<GLint, 2> maxViewport{};
  glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTexture);
  glGetIntegerv(GL_MAX_RENDERBUFFER_SIZE, &maxRenderbuffer);
  glGetIntegerv(GL_MAX_VIEWPORT_DIMS, maxViewport.data());
  const auto maxWidth = std::min({maxTexture, maxRenderbuffer, maxViewport[0]});
  const auto maxHeight = std::min({maxTexture, maxRenderbuffer, maxViewport[1]});
  if (size.width > maxWidth || size.height > maxHeight) {
    std::ostringstream message;
    message << "Offscreen target " << size.width << 'x' << size.height << " exceeds GPU size limit "
            << maxWidth << 'x' << maxHeight << '.';
    throw std::invalid_argument(message.str());
  }
  const auto width = static_cast<std::size_t>(size.width);
  const auto height = static_cast<std::size_t>(size.height);
  if (width > std::numeric_limits<std::size_t>::max() / 4U / height) {
    throw std::overflow_error("Offscreen RGBA pixel buffer size overflows size_t.");
  }

  const AllocationState state;
  glGenFramebuffers(1, &m_framebuffer);
  glGenTextures(1, &m_colorTexture);
  glBindTexture(GL_TEXTURE_2D, m_colorTexture);
  glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexImage2D(
    GL_TEXTURE_2D, 0, GL_RGBA8, size.width, size.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
  glBindFramebuffer(GL_FRAMEBUFFER, m_framebuffer);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_colorTexture, 0);
  glDrawBuffer(GL_COLOR_ATTACHMENT0);
  glReadBuffer(GL_COLOR_ATTACHMENT0);
  const auto status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
  const auto error = glGetError();
  if (m_framebuffer == 0 || m_colorTexture == 0 || status != GL_FRAMEBUFFER_COMPLETE ||
      error != GL_NO_ERROR) {
    glDeleteFramebuffers(1, &m_framebuffer);
    glDeleteTextures(1, &m_colorTexture);
    std::ostringstream message;
    message << "Could not allocate offscreen target " << size.width << 'x' << size.height
            << ": framebuffer status 0x" << std::hex << status << ", OpenGL error 0x" << error
            << '.';
    throw std::runtime_error(message.str());
  }
}

OffscreenRenderTarget::~OffscreenRenderTarget()
{
  glDeleteFramebuffers(1, &m_framebuffer);
  glDeleteTextures(1, &m_colorTexture);
}

FramebufferSize OffscreenRenderTarget::size() const
{
  return m_size;
}

unsigned int OffscreenRenderTarget::framebuffer() const
{
  return m_framebuffer;
}

OffscreenPixelBuffer OffscreenRenderTarget::readPixels() const
{
  OffscreenPixelBuffer result;
  result.size = m_size;
  result.strideBytes = static_cast<std::size_t>(m_size.width) * 4U;
  result.pixels.resize(result.strideBytes * static_cast<std::size_t>(m_size.height));

  const ReadbackState state;
  glBindFramebuffer(GL_READ_FRAMEBUFFER, m_framebuffer);
  glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
  glPixelStorei(GL_PACK_ALIGNMENT, 1);
  glPixelStorei(GL_PACK_ROW_LENGTH, 0);
  glPixelStorei(GL_PACK_SKIP_ROWS, 0);
  glPixelStorei(GL_PACK_SKIP_PIXELS, 0);
  glReadPixels(0, 0, m_size.width, m_size.height, GL_RGBA, GL_UNSIGNED_BYTE, result.pixels.data());
  if (const auto error = glGetError(); error != GL_NO_ERROR) {
    std::ostringstream message;
    message << "Offscreen RGBA readback failed for " << m_size.width << 'x' << m_size.height
            << ": OpenGL error 0x" << std::hex << error << '.';
    throw std::runtime_error(message.str());
  }
  const std::span<std::uint8_t> pixels{result.pixels};
  const auto height = static_cast<std::size_t>(m_size.height);
  for (std::size_t y = 0; y < height / 2; ++y) {
    const auto top = pixels.subspan(y * result.strideBytes, result.strideBytes);
    const auto bottom = pixels.subspan((height - 1 - y) * result.strideBytes, result.strideBytes);
    std::ranges::swap_ranges(top, bottom);
  }
  return result;
}
