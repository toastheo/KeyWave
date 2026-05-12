#include "ui/TransportControls.hpp"

#include <imgui.h>

#include "app/PlaybackTransportAction.hpp"
#include "ui/TransportControlsConfig.hpp"
#include "ui/TransportTime.hpp"

void TransportControls::render(PlaybackTransport& transport, const double durationSeconds)
{
  const ImGuiViewport* viewport = ImGui::GetMainViewport();
  const ImVec2 position{viewport->WorkPos.x + 16.0f,
                        viewport->WorkPos.y + viewport->WorkSize.y - 96.0f};

  ImGui::SetNextWindowPos(position, ImGuiCond_Always);
  ImGui::SetNextWindowBgAlpha(0.88f);

  if (!ImGui::Begin("Transport Controls", nullptr, transportControlsWindowFlags())) {
    ImGui::End();
    return;
  }

  if (ImGui::Button("<< 5s")) {
    applyPlaybackTransportAction(PlaybackTransportAction::SeekBackwardFiveSeconds, transport);
  }

  ImGui::SameLine();
  const char* playPauseLabel = transport.state() == PlaybackState::Playing ? "Pause" : "Play";
  if (ImGui::Button(playPauseLabel)) {
    applyPlaybackTransportAction(PlaybackTransportAction::TogglePlayPause, transport);
  }

  ImGui::SameLine();
  if (ImGui::Button("Stop")) {
    applyPlaybackTransportAction(PlaybackTransportAction::Stop, transport);
  }

  ImGui::SameLine();
  if (ImGui::Button("5s >>")) {
    applyPlaybackTransportAction(PlaybackTransportAction::SeekForwardFiveSeconds, transport);
  }

  ImGui::SameLine();
  ImGui::Text("Rate %.2fx", transport.playbackRate());

  const auto clampedTime = clampTransportPosition(transport.currentTimeSeconds(), durationSeconds);
  double currentTime = clampedTime;
  constexpr double minimumTime = 0.0;
  const double maximumTime = durationSeconds > 0.0 ? durationSeconds : 0.0;

  ImGui::Text("%s / %s",
              formatTransportTime(clampedTime).c_str(),
              formatTransportTime(maximumTime).c_str());

  if (durationSeconds <= 0.0) {
    ImGui::BeginDisabled();
  }

  ImGui::SetNextItemWidth(360.0f);
  if (ImGui::SliderScalar("##PlaybackPosition",
                          ImGuiDataType_Double,
                          &currentTime,
                          &minimumTime,
                          &maximumTime,
                          "%.2f s")) {
    transport.seek(clampTransportPosition(currentTime, durationSeconds));
  }

  if (durationSeconds <= 0.0) {
    ImGui::EndDisabled();
  }

  ImGui::End();
}
