#include <catch2/catch_test_macros.hpp>
#include <stdexcept>

#include "export/VideoExportSettings.hpp"

namespace {

TEST_CASE("Export settings map all format presets independently of quality and FPS", "[export]")
{
  struct Expected
  {
    ExportResolution resolution;
    ExportAspectRatio aspect;
    int width;
    int height;
  };
  for (const auto& row : {Expected{.resolution = ExportResolution::P720,
                                   .aspect = ExportAspectRatio::Landscape,
                                   .width = 1280,
                                   .height = 720},
                          Expected{.resolution = ExportResolution::P720,
                                   .aspect = ExportAspectRatio::Portrait,
                                   .width = 720,
                                   .height = 1280},
                          Expected{.resolution = ExportResolution::P720,
                                   .aspect = ExportAspectRatio::Square,
                                   .width = 720,
                                   .height = 720},
                          Expected{.resolution = ExportResolution::P1080,
                                   .aspect = ExportAspectRatio::Landscape,
                                   .width = 1920,
                                   .height = 1080},
                          Expected{.resolution = ExportResolution::P1080,
                                   .aspect = ExportAspectRatio::Portrait,
                                   .width = 1080,
                                   .height = 1920},
                          Expected{.resolution = ExportResolution::P1080,
                                   .aspect = ExportAspectRatio::Square,
                                   .width = 1080,
                                   .height = 1080},
                          Expected{.resolution = ExportResolution::P2160,
                                   .aspect = ExportAspectRatio::Landscape,
                                   .width = 3840,
                                   .height = 2160},
                          Expected{.resolution = ExportResolution::P2160,
                                   .aspect = ExportAspectRatio::Portrait,
                                   .width = 2160,
                                   .height = 3840},
                          Expected{.resolution = ExportResolution::P2160,
                                   .aspect = ExportAspectRatio::Square,
                                   .width = 2160,
                                   .height = 2160}}) {
    for (const auto fps : {30, 60}) {
      for (const auto quality : {ExportQuality::Medium, ExportQuality::High, ExportQuality::Best}) {
        VideoExportSettings settings;
        settings.fps = fps;
        settings.quality = quality;
        settings.resolution = row.resolution;
        settings.aspectRatio = row.aspect;
        const auto size = exportFramebufferSize(settings);
        CHECK(size.width == row.width);
        CHECK(size.height == row.height);
      }
    }
  }
}

TEST_CASE("Export settings reject unsupported options instead of silently substituting defaults",
          "[export]")
{
  VideoExportSettings settings;
  SECTION("FPS")
  {
    settings.fps = 24;
  }
  SECTION("quality")
  {
    settings.quality = static_cast<ExportQuality>(99);
  }
  SECTION("resolution")
  {
    settings.resolution = static_cast<ExportResolution>(99);
  }
  SECTION("aspect")
  {
    settings.aspectRatio = static_cast<ExportAspectRatio>(99);
  }
  SECTION("range mode")
  {
    settings.range.mode = static_cast<ExportRangeMode>(99);
  }
  CHECK_THROWS_AS(exportFramebufferSize(settings), std::invalid_argument);
}

} // namespace
