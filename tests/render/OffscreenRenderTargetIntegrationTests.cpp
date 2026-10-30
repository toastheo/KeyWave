#include <catch2/catch_test_macros.hpp>
#include <cstddef>
#include <cstdint>
#include <glad/gl.h>
#include <utility>

#include "render/RenderCommand.hpp"
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <array>
#include <limits>
#include <stdexcept>

#include "render/RenderScene.hpp"
#include "render_opengl/OffscreenRenderTarget.hpp"
#include "render_opengl/OpenGLRendererBackend.hpp"

namespace {

struct Context
{
  GLFWwindow* window = nullptr;

  Context()
  {
    if (glfwInit() == GLFW_FALSE) {
      throw std::runtime_error("GPU tests require GLFW and an OpenGL 3.3 desktop context");
    }
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    window = glfwCreateWindow(160, 90, "KeyWave GPU test", nullptr, nullptr);
    if (window == nullptr) {
      glfwTerminate();
      throw std::runtime_error("GPU tests could not create an OpenGL 3.3 context");
    }
    glfwMakeContextCurrent(window);
    if (gladLoadGL(glfwGetProcAddress) == 0) {
      glfwDestroyWindow(window);
      glfwTerminate();
      throw std::runtime_error("GPU tests could not load OpenGL");
    }
  }

  ~Context()
  {
    glfwDestroyWindow(window);
    glfwTerminate();
  }
};

std::array<std::uint8_t, 4> pixel(const OffscreenPixelBuffer& image, int x, int y)
{
  const auto offset =
    static_cast<std::size_t>(y) * image.strideBytes + static_cast<std::size_t>(x) * 4U;
  return {image.pixels.at(offset),
          image.pixels.at(offset + 1),
          image.pixels.at(offset + 2),
          image.pixels.at(offset + 3)};
}

RenderScene asymmetricScene()
{
  RenderScene scene;
  scene.view.visibleWorldRect = {.x = 0, .y = 0, .width = 4, .height = 4};
  // World Y grows upwards; the pixel buffer starts at the top.
  scene.commands.emplace_back(DrawRectCommand{.rect = {.x = 0, .y = 3, .width = 2, .height = 1},
                                              .color = {.r = 1, .g = 0, .b = 0, .a = 1}});
  scene.commands.emplace_back(DrawRectCommand{.rect = {.x = 2, .y = 0, .width = 2, .height = 1},
                                              .color = {.r = 0, .g = 1, .b = 0, .a = 1}});
  return scene;
}

TEST_CASE("Offscreen rendering returns tightly packed top-first RGBA at export size",
          "[gpu][offscreen]")
{
  Context const context;
  OpenGLRendererBackend renderer(glfwGetProcAddress, {.r = 0, .g = 0, .b = 1, .a = 1});
  renderer.setFramebufferSize({.width = 160, .height = 90});
  REQUIRE(renderer.initialize());
  OffscreenRenderTarget target({.width = 1920, .height = 1080});
  const auto image =
    renderer.renderOffscreen(target, asymmetricScene(), {.r = 0, .g = 0, .b = 1, .a = 1});
  CHECK(image.size.width == 1920);
  CHECK(image.size.height == 1080);
  CHECK(image.strideBytes == std::size_t{1920} * 4);
  CHECK(image.pixels.size() == image.strideBytes * 1080U);
  CHECK(pixel(image, 100, 100) == std::array<std::uint8_t, 4>{255, 0, 0, 255});
  CHECK(pixel(image, 1800, 1000) == std::array<std::uint8_t, 4>{0, 255, 0, 255});
  CHECK(pixel(image, 1800, 100) == std::array<std::uint8_t, 4>{0, 0, 255, 255});
  CHECK(glGetError() == GL_NO_ERROR);
}

TEST_CASE("Offscreen operations restore framebuffer viewport packing and live renderer",
          "[gpu][offscreen]")
{
  Context const context;
  OpenGLRendererBackend renderer(glfwGetProcAddress, {.r = 1, .g = 0, .b = 1, .a = 1});
  renderer.setFramebufferSize({.width = 160, .height = 90});
  renderer.setView({{.x = 0, .y = 0, .width = 8, .height = 8}});
  REQUIRE(renderer.initialize());
  OffscreenRenderTarget const live({.width = 160, .height = 90});
  glBindFramebuffer(GL_DRAW_FRAMEBUFFER, live.framebuffer());
  glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
  glViewport(3, 4, 150, 80);
  glEnable(GL_SCISSOR_TEST);
  glScissor(0, 0, 1, 1);
  glPixelStorei(GL_PACK_ALIGNMENT, 8);
  glPixelStorei(GL_PACK_ROW_LENGTH, 2000);
  glPixelStorei(GL_PACK_SKIP_ROWS, 2);
  glPixelStorei(GL_PACK_SKIP_PIXELS, 3);
  GLuint packBuffer = 0;
  glGenBuffers(1, &packBuffer);
  glBindBuffer(GL_PIXEL_PACK_BUFFER, packBuffer);
  {
    OffscreenRenderTarget target({.width = 1919, .height = 1079});
    const auto image =
      renderer.renderOffscreen(target, asymmetricScene(), {.r = 0, .g = 0, .b = 1, .a = 1});
    CHECK(pixel(image, 10, 10) == std::array<std::uint8_t, 4>{255, 0, 0, 255});
  }
  GLint value = 0;
  glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &value);
  CHECK(std::cmp_equal(value, live.framebuffer()));
  glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &value);
  CHECK(value == 0);
  std::array<GLint, 4> viewport{};
  glGetIntegerv(GL_VIEWPORT, viewport.data());
  CHECK(viewport == std::array<GLint, 4>{3, 4, 150, 80});
  CHECK(glIsEnabled(GL_SCISSOR_TEST) == GL_TRUE);
  glDisable(GL_SCISSOR_TEST);
  glGetIntegerv(GL_PACK_ALIGNMENT, &value);
  CHECK(value == 8);
  glGetIntegerv(GL_PACK_ROW_LENGTH, &value);
  CHECK(value == 2000);
  glGetIntegerv(GL_PACK_SKIP_ROWS, &value);
  CHECK(value == 2);
  glGetIntegerv(GL_PACK_SKIP_PIXELS, &value);
  CHECK(value == 3);
  glGetIntegerv(GL_PIXEL_PACK_BUFFER_BINDING, &value);
  CHECK(std::cmp_equal(value, packBuffer));
  glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
  glDeleteBuffers(1, &packBuffer);
  glViewport(0, 0, 160, 90);
  renderer.beginFrame();
  const std::array<RenderCommand, 1> line{DrawLineCommand{.from = {.x = 0, .y = 4},
                                                          .to = {.x = 8, .y = 4},
                                                          .color = {.r = 1, .g = 1, .b = 1, .a = 1},
                                                          .thickness = 2}};
  renderer.submit(line);
  renderer.endFrame();
  const auto image = live.readPixels();
  CHECK(pixel(image, 10, 10) == std::array<std::uint8_t, 4>{255, 0, 255, 255});
  CHECK(pixel(image, 80, 44) == std::array<std::uint8_t, 4>{255, 255, 255, 255});
  CHECK(pixel(image, 80, 40) == std::array<std::uint8_t, 4>{255, 0, 255, 255});
  CHECK(glGetError() == GL_NO_ERROR);
}

TEST_CASE("Offscreen allocation preserves bindings and releases owned GPU resources",
          "[gpu][offscreen]")
{
  Context const context;
  GLuint texture = 0;
  GLuint unpackBuffer = 0;
  glGenTextures(1, &texture);
  glBindTexture(GL_TEXTURE_2D, texture);
  glGenBuffers(1, &unpackBuffer);
  glBindBuffer(GL_PIXEL_UNPACK_BUFFER, unpackBuffer);
  GLuint framebuffer = 0;
  GLuint colorTexture = 0;
  {
    OffscreenRenderTarget const target({.width = 32, .height = 24});
    framebuffer = target.framebuffer();
    CHECK(glIsFramebuffer(framebuffer) == GL_TRUE);
    GLint value = 0;
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &value);
    CHECK(std::cmp_equal(value, texture));
    glGetIntegerv(GL_PIXEL_UNPACK_BUFFER_BINDING, &value);
    CHECK(std::cmp_equal(value, unpackBuffer));
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    glGetFramebufferAttachmentParameteriv(
      GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME, &value);
    colorTexture = static_cast<GLuint>(value);
    CHECK(glIsTexture(colorTexture) == GL_TRUE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
  }
  CHECK(glIsFramebuffer(framebuffer) == GL_FALSE);
  CHECK(glIsTexture(colorTexture) == GL_FALSE);
  glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
  glDeleteBuffers(1, &unpackBuffer);
  glBindTexture(GL_TEXTURE_2D, 0);
  glDeleteTextures(1, &texture);
  CHECK(glGetError() == GL_NO_ERROR);
}

TEST_CASE("Offscreen rendering rejects unavailable renderer and invalid scene", "[gpu][offscreen]")
{
  Context const context;
  OffscreenRenderTarget target({.width = 32, .height = 24});
  OpenGLRendererBackend renderer(glfwGetProcAddress, {.r = 0, .g = 0, .b = 0, .a = 1});
  CHECK_THROWS_AS(renderer.renderOffscreen(target, asymmetricScene(), {0, 0, 0, 1}),
                  std::logic_error);
  renderer.setFramebufferSize({.width = 160, .height = 90});
  REQUIRE(renderer.initialize());
  auto scene = asymmetricScene();
  scene.view.visibleWorldRect.width = 0;
  CHECK_THROWS_AS(renderer.renderOffscreen(target, scene, {0, 0, 0, 1}), std::invalid_argument);
  GLint framebuffer = -1;
  glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &framebuffer);
  CHECK(framebuffer == 0);
  CHECK(glGetError() == GL_NO_ERROR);
}

TEST_CASE("Offscreen readback reports GPU errors and restores caller state", "[gpu][offscreen]")
{
  Context const context;
  OffscreenRenderTarget const target({.width = 32, .height = 24});
  // Simulate a target becoming incomplete: readback must fail, not return a
  // zero-filled buffer that an encoder would accept as a successful frame.
  glBindFramebuffer(GL_FRAMEBUFFER, target.framebuffer());
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 0, 0);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glPixelStorei(GL_PACK_ALIGNMENT, 8);
  CHECK_THROWS_AS(target.readPixels(), std::runtime_error);
  GLint value = -1;
  glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &value);
  CHECK(value == 0);
  glGetIntegerv(GL_PACK_ALIGNMENT, &value);
  CHECK(value == 8);
  CHECK(glGetError() == GL_NO_ERROR);
}

TEST_CASE("Offscreen targets reject invalid and excessive GPU sizes and support 4K",
          "[gpu][offscreen]")
{
  Context const context;
  CHECK_THROWS_AS(OffscreenRenderTarget(FramebufferSize{0, 1}), std::invalid_argument);
  CHECK_THROWS_AS(OffscreenRenderTarget(FramebufferSize{-1, 1}), std::invalid_argument);
  CHECK_THROWS_AS(OffscreenRenderTarget(FramebufferSize{std::numeric_limits<int>::max(), 1}),
                  std::invalid_argument);
  GLint maxTexture = 0;
  GLint maxRenderbuffer = 0;
  std::array<GLint, 2> maxViewport{};
  glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTexture);
  glGetIntegerv(GL_MAX_RENDERBUFFER_SIZE, &maxRenderbuffer);
  glGetIntegerv(GL_MAX_VIEWPORT_DIMS, maxViewport.data());
  if (maxTexture >= 3840 && maxRenderbuffer >= 3840 && maxViewport[0] >= 3840 &&
      maxViewport[1] >= 2160) {
    OpenGLRendererBackend renderer(glfwGetProcAddress, {.r = 0, .g = 0, .b = 0, .a = 1});
    REQUIRE(renderer.initialize());
    OffscreenRenderTarget target({.width = 3840, .height = 2160});
    const auto image =
      renderer.renderOffscreen(target, asymmetricScene(), {.r = 0, .g = 0, .b = 1, .a = 1});
    CHECK(pixel(image, 100, 100) == std::array<std::uint8_t, 4>{255, 0, 0, 255});
    CHECK(pixel(image, 3800, 2100) == std::array<std::uint8_t, 4>{0, 255, 0, 255});
  }
  else {
    CHECK_THROWS_AS(OffscreenRenderTarget(FramebufferSize{3840, 2160}), std::invalid_argument);
  }
  CHECK(glGetError() == GL_NO_ERROR);
}

} // namespace
