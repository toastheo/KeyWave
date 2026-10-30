#include "ui/VideoExportControls.hpp"

#include <algorithm>
#include <imgui.h>

#include "app/VideoExportController.hpp"
#include "export/VideoEncoder.hpp"

VideoExportAction VideoExportControls::render(const VideoExportController& controller,
                                              bool hasTimeline)
{
  ImGui::SetNextWindowPos({16, 16}, ImGuiCond_FirstUseEver);
  ImGui::SetNextWindowSize({340, 0}, ImGuiCond_FirstUseEver);
  const bool visible = ImGui::Begin("Video export", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
  VideoExportAction action = VideoExportAction::None;
  if (visible) {
    const auto status = controller.status();
    const bool running = controller.active();
    ImGui::TextUnformatted("Full song | 1920 x 1080 | 30 FPS | No audio");
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