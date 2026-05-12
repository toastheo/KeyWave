#include "ui/TransportControls.hpp"

#include <imgui.h>

#include "app/PlaybackTransportAction.hpp"

void TransportControls::render(PlaybackTransport& transport)
{
  const ImGuiViewport* viewport = ImGui::GetMainViewport();
  const ImVec2 position{viewport->WorkPos.x + 16.0f,
                        viewport->WorkPos.y + viewport->WorkSize.y - 64.0f};

  ImGui::SetNextWindowPos(position, ImGuiCond_Always);
  ImGui::SetNextWindowBgAlpha(0.88f);

  constexpr ImGuiWindowFlags kWindowFlags =
    ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize |
    ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoMove;

  if (!ImGui::Begin("Transport Controls", nullptr, kWindowFlags)) {
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
  ImGui::Text("Time %.2fs", transport.currentTimeSeconds());

  ImGui::SameLine();
  ImGui::Text("Rate %.2fx", transport.playbackRate());

  ImGui::End();
}
