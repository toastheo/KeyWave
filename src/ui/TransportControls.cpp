#include "ui/TransportControls.hpp"

#include <imgui.h>
#include <sstream>
#include <string>

#include "app/AppSettings.hpp"
#include "app/PlaybackTransportAction.hpp"
#include "playback/PlaybackTransport.hpp"
#include "ui/TransportControlsConfig.hpp"
#include "ui/TransportTime.hpp"

namespace {

std::string formatSeekStepLabel(const double seekStepSeconds)
{
  std::ostringstream output;
  output << seekStepSeconds << "s";
  return output.str();
}

} // namespace

void TransportControls::render(PlaybackTransport& transport,
                               const double durationSeconds,
                               const PlaybackControlSettings& settings,
                               const double sourceBpm)
{
  const auto sanitizedSettings = sanitizePlaybackControlSettings(settings);
  const ImGuiViewport* viewport = ImGui::GetMainViewport();
  const ImVec2 position{viewport->WorkPos.x + 16.0f,
                        viewport->WorkPos.y + viewport->WorkSize.y - 96.0f};

  ImGui::SetNextWindowPos(position, ImGuiCond_Always);
  ImGui::SetNextWindowBgAlpha(0.88f);

  if (!ImGui::Begin("Transport Controls", nullptr, transportControlsWindowFlags())) {
    ImGui::End();
    return;
  }

  const auto seekLabel = formatSeekStepLabel(sanitizedSettings.seekStepSeconds);
  const auto seekBackwardLabel = "<< " + seekLabel;
  const auto seekForwardLabel = seekLabel + " >>";

  if (ImGui::Button(seekBackwardLabel.c_str())) {
    applyPlaybackTransportAction(PlaybackTransportAction::SeekBackward,
                                 transport,
                                 sanitizedSettings);
  }

  ImGui::SameLine();
  const char* playPauseLabel = transport.state() == PlaybackState::Playing ? "Pause" : "Play";
  if (ImGui::Button(playPauseLabel)) {
    applyPlaybackTransportAction(PlaybackTransportAction::TogglePlayPause,
                                 transport,
                                 sanitizedSettings);
  }

  ImGui::SameLine();
  if (ImGui::Button("Stop")) {
    applyPlaybackTransportAction(PlaybackTransportAction::Stop, transport, sanitizedSettings);
  }

  ImGui::SameLine();
  if (ImGui::Button(seekForwardLabel.c_str())) {
    applyPlaybackTransportAction(PlaybackTransportAction::SeekForward,
                                 transport,
                                 sanitizedSettings);
  }

  ImGui::SameLine();
  ImGui::Text("BPM %.0f", transport.effectiveBpm(sourceBpm));

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
