#include "ui/TransportControls.hpp"

#include <imgui.h>
#include <sstream>
#include <string>

#include "app/AppSettings.hpp"
#include "app/PlaybackTransportAction.hpp"
#include "audio/TimelineAudioScheduler.hpp"
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

ImGuiWindowFlags toImGuiWindowFlags(const TransportControlsWindowFlags flags)
{
  ImGuiWindowFlags imguiFlags = 0;
  if (hasTransportControlsWindowFlag(flags, TransportControlsWindowFlag::NoDecoration)) {
    imguiFlags |= ImGuiWindowFlags_NoDecoration;
  }
  if (hasTransportControlsWindowFlag(flags, TransportControlsWindowFlag::AlwaysAutoResize)) {
    imguiFlags |= ImGuiWindowFlags_AlwaysAutoResize;
  }
  if (hasTransportControlsWindowFlag(flags, TransportControlsWindowFlag::NoSavedSettings)) {
    imguiFlags |= ImGuiWindowFlags_NoSavedSettings;
  }
  if (hasTransportControlsWindowFlag(flags, TransportControlsWindowFlag::NoMove)) {
    imguiFlags |= ImGuiWindowFlags_NoMove;
  }
  if (hasTransportControlsWindowFlag(flags, TransportControlsWindowFlag::NoFocusOnAppearing)) {
    imguiFlags |= ImGuiWindowFlags_NoFocusOnAppearing;
  }
  if (hasTransportControlsWindowFlag(flags, TransportControlsWindowFlag::NoNavFocus)) {
    imguiFlags |= ImGuiWindowFlags_NoNavFocus;
  }
  if (hasTransportControlsWindowFlag(flags, TransportControlsWindowFlag::NoMouseInputs)) {
    imguiFlags |= ImGuiWindowFlags_NoMouseInputs;
  }

  return imguiFlags;
}

void applyTransportAction(const PlaybackTransportAction& action,
                          PlaybackTransport& transport,
                          const PlaybackControlSettings& settings,
                          const double sourceBpm,
                          TimelineAudioScheduler& audioScheduler)
{
  applyPlaybackTransportAction(action, transport, audioScheduler, settings, sourceBpm);
}

void seekTransport(PlaybackTransport& transport,
                   const double timeSeconds,
                   TimelineAudioScheduler& audioScheduler)
{
  transport.seek(timeSeconds);
  audioScheduler.seek(transport.currentTimeSeconds());
}

void renderTransportControls(PlaybackTransport& transport,
                             const double durationSeconds,
                             const PlaybackControlSettings& settings,
                             const double sourceBpm,
                             TimelineAudioScheduler& audioScheduler)
{
  const auto sanitizedSettings = sanitizePlaybackControlSettings(settings);
  const ImGuiViewport* viewport = ImGui::GetMainViewport();
  const ImVec2 position{viewport->WorkPos.x + 16.0f,
                        viewport->WorkPos.y + viewport->WorkSize.y - 96.0f};

  ImGui::SetNextWindowPos(position, ImGuiCond_Always);
  ImGui::SetNextWindowBgAlpha(0.88f);

  if (!ImGui::Begin(
        "Transport Controls", nullptr, toImGuiWindowFlags(transportControlsWindowFlags()))) {
    ImGui::End();
    return;
  }

  const auto seekLabel = formatSeekStepLabel(sanitizedSettings.seekStepSeconds);
  const auto seekBackwardLabel = "<< " + seekLabel;
  const auto seekForwardLabel = seekLabel + " >>";

  if (ImGui::Button(seekBackwardLabel.c_str())) {
    applyTransportAction(PlaybackTransportAction::SeekBackward,
                         transport,
                         sanitizedSettings,
                         sourceBpm,
                         audioScheduler);
  }

  ImGui::SameLine();
  const char* playPauseLabel = transport.state() == PlaybackState::Playing ? "Pause" : "Play";
  if (ImGui::Button(playPauseLabel)) {
    applyTransportAction(PlaybackTransportAction::TogglePlayPause,
                         transport,
                         sanitizedSettings,
                         sourceBpm,
                         audioScheduler);
  }

  ImGui::SameLine();
  if (ImGui::Button("Stop")) {
    applyTransportAction(
      PlaybackTransportAction::Stop, transport, sanitizedSettings, sourceBpm, audioScheduler);
  }

  ImGui::SameLine();
  if (ImGui::Button(seekForwardLabel.c_str())) {
    applyTransportAction(PlaybackTransportAction::SeekForward,
                         transport,
                         sanitizedSettings,
                         sourceBpm,
                         audioScheduler);
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
    seekTransport(transport, clampTransportPosition(currentTime, durationSeconds), audioScheduler);
  }

  if (durationSeconds <= 0.0) {
    ImGui::EndDisabled();
  }

  ImGui::End();
}

} // namespace

void TransportControls::render(PlaybackTransport& transport,
                               const double durationSeconds,
                               TimelineAudioScheduler& audioScheduler,
                               const PlaybackControlSettings& settings,
                               const double sourceBpm)
{
  renderTransportControls(transport, durationSeconds, settings, sourceBpm, audioScheduler);
}
