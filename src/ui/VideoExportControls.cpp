#include "ui/VideoExportControls.hpp"

#include <algorithm>
#include <exception>
#include <imgui.h>

#include "app/VideoExportController.hpp"
#include "core/CoreTypes.hpp"
#include "export/VideoEncoder.hpp"
#include "export/VideoExportSettings.hpp"

VideoExportAction VideoExportControls::render(const VideoExportController& controller,
                                              bool hasTimeline,
                                              VideoExportSettings& settings,
                                              const FrameTextureProvider& frameTexture)
{
  ImGui::SetNextWindowPos({16, 16}, ImGuiCond_FirstUseEver);
  ImGui::SetNextWindowSize({340, 0}, ImGuiCond_FirstUseEver);
  const bool visible = ImGui::Begin("Video export", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
  VideoExportAction action = VideoExportAction::None;
  if (visible) {
    const auto status = controller.status();
    const bool running = controller.active();

    ImGui::TextUnformatted("Full song | no audio");
    ImGui::BeginDisabled(running);

    int fps = settings.fps == 30 ? 0 : 1;
    if (ImGui::Combo("Framerate",
                     &fps,
                     "30 FPS\0"
                     "60 FPS\0")) {
      settings.fps = fps == 0 ? 30 : 60;
    }
    int quality = static_cast<int>(settings.quality);
    if (ImGui::Combo("Quality", &quality, "Medium\0High\0Best\0")) {
      settings.quality = static_cast<ExportQuality>(quality);
    }
    int resolution = static_cast<int>(settings.resolution);
    if (ImGui::Combo("Resolution",
                     &resolution,
                     "720p\0"
                     "1080p\0"
                     "4K\0")) {
      settings.resolution = static_cast<ExportResolution>(resolution);
    }
    int aspect = static_cast<int>(settings.aspectRatio);
    if (ImGui::Combo("Aspect ratio",
                     &aspect,
                     "16:9\0"
                     "9:16\0"
                     "1:1\0")) {
      settings.aspectRatio = static_cast<ExportAspectRatio>(aspect);
    }
    ImGui::EndDisabled();
    const auto size = exportFramebufferSize(settings);
    ImGui::Text("%d x %d pixels", size.width, size.height);
    if (hasTimeline && ImGui::CollapsingHeader("Export frame", ImGuiTreeNodeFlags_DefaultOpen)) {
      if (!running) {
        ImGui::TextUnformatted("Generated frames appear here while exporting.");
      }
      else {
        const FramebufferSize previewSize =
          size.width > size.height   ? FramebufferSize{.width = 288, .height = 162}
          : size.width < size.height ? FramebufferSize{.width = 162, .height = 288}
                                     : FramebufferSize{.width = 288, .height = 288};

        try {
          // Borrow the exact texture used for the latest generated encoder frame.
          const auto texture = frameTexture(size);
          if (texture != 0) {
            ImGui::Image(static_cast<ImTextureID>(texture),
                         {static_cast<float>(previewSize.width),
                          static_cast<float>(previewSize.height)},
                         {0, 1},
                         {1, 0});
          }
          else {
            ImGui::TextUnformatted("Waiting for the first export frame...");
          }
        }
        catch (const std::exception& error) {
          ImGui::TextWrapped("Preview unavailable: %s", error.what());
        }
        ImGui::Text("Generated frame %llu of %llu",
                    static_cast<unsigned long long>(controller.framesGenerated()),
                    static_cast<unsigned long long>(controller.frameCount()));
      }
    }

    ImGui::BeginDisabled(running || !hasTimeline);
    if (ImGui::Button("Export MP4...")) {
      action = VideoExportAction::Start;
    }
    ImGui::EndDisabled();
    if (running) {
      ImGui::SameLine();
      if (ImGui::Button("Cancel")) {
        action = VideoExportAction::Cancel;
      }
      const auto count = controller.frameCount();
      const float progress = count == 0
                               ? 0.0f
                               : static_cast<float>(static_cast<double>(status.framesWritten) /
                                                    static_cast<double>(count));
      ImGui::ProgressBar(std::clamp(progress, 0.0f, 1.0f));
      ImGui::Text("Duration %.2f s", controller.outputDurationSeconds());
    }
    switch (status.state) {
      case VideoEncoderState::Starting:
        ImGui::TextUnformatted("Preparing export...");
        break;
      case VideoEncoderState::Running:
        ImGui::TextUnformatted("Exporting frame...");
        break;
      case VideoEncoderState::Finalizing:
        ImGui::TextUnformatted("Finalizing MP4...");
        break;
      case VideoEncoderState::Completed: {
        const auto path = controller.destination().u8string();
        ImGui::TextWrapped("Saved: %s", reinterpret_cast<const char*>(path.c_str()));
        break;
      }
      case VideoEncoderState::Cancelled:
        ImGui::TextUnformatted("Export cancelled.");
        break;
      case VideoEncoderState::Failed:
        ImGui::TextWrapped("Export failed: %s", status.error.c_str());
        break;
      case VideoEncoderState::Idle:
        break;
    }
    if (!hasTimeline && !running) {
      ImGui::TextUnformatted("Load a MIDI file to export.");
    }
  }
  ImGui::End();
  return action;
}
