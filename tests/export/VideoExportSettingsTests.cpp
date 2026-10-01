#include <catch2/catch_test_macros.hpp>
#include <stdexcept>

#include "export/VideoExportSettings.hpp"

TEST_CASE("Export settings map all format presets independently of quality and FPS", "[export]")
{
  struct Expected
  {
    ExportResolution resolution;
    ExportAspectRatio aspect;
    int width;
    int height;
  };
  for (const auto& row :
       {Expected{ExportResolution::P720, ExportAspectRatio::Landscape, 1280, 720},
        Expected{ExportResolution::P720, ExportAspectRatio::Portrait, 720, 1280},
        Expected{ExportResolution::P720, ExportAspectRatio::Square, 720, 720},
        Expected{ExportResolution::P1080, ExportAspectRatio::Landscape, 1920, 1080},
        Expected{ExportResolution::P1080, ExportAspectRatio::Portrait, 1080, 1920},
        Expected{ExportResolution::P1080, ExportAspectRatio::Square, 1080, 1080},
        Expected{ExportResolution::P2160, ExportAspectRatio::Landscape, 3840, 2160},
        Expected{ExportResolution::P2160, ExportAspectRatio::Portrait, 2160, 3840},
        Expected{ExportResolution::P2160, ExportAspectRatio::Square, 2160, 2160}}) {
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
