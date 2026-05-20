#include "ui/VisualizationSettingsPanel.hpp"

#include <imgui.h>

#include <algorithm>
#include <array>
#include <cmath>

namespace {

double finiteOr(const double value, const double fallback)
{
  return std::isfinite(value) ? value : fallback;
}

double clampRange(const double value, const double minimum, const double maximum)
{
  return std::clamp(finiteOr(value, minimum), minimum, maximum);
}

double clampPositive(const double value, const double fallback)
{
  constexpr double kMinimumPositive = 0.000001;
  return std::max(kMinimumPositive, finiteOr(value, fallback));
}

bool editDoubleSlider(const char* label, double& value, const double minimum, const double maximum)
{
  auto editableValue = static_cast<float>(clampRange(value, minimum, maximum));
  const auto changed =
    ImGui::SliderFloat(label, &editableValue, static_cast<float>(minimum), static_cast<float>(maximum));
  value = clampRange(editableValue, minimum, maximum);
  return changed;
}

bool editSeparatorWidth(double& value)
{
  constexpr double kDragSpeed = 0.05;
  constexpr double kMinimum = 0.0;
  constexpr double kMaximum = 8.0;

  auto editableValue = static_cast<float>(clampRange(value, kMinimum, kMaximum));
  const auto changed = ImGui::DragFloat("Separator Width",
                                        &editableValue,
                                        kDragSpeed,
                                        kMinimum,
                                        kMaximum);
  value = clampRange(editableValue, kMinimum, kMaximum);
  return changed;
}

bool editColor(const char* label, Color& color)
{
  std::array values{color.r, color.g, color.b, color.a};
  if (!ImGui::ColorEdit4(label, values.data())) {
    return false;
  }

  color = Color{
    .r = values[0],
    .g = values[1],
    .b = values[2],
    .a = values[3],
  };
  return true;
}

void renderPlaybackSettings(AppSettings const& settings, PlaybackTransport& transport)
{
  const auto playbackSettings = sanitizePlaybackControlSettings(settings.playbackControls);
  const auto minimumRate = playbackSettings.minPlaybackRate;
  const auto maximumRate = playbackSettings.maxPlaybackRate;

  auto playbackRate = static_cast<float>(
    clampRange(transport.playbackRate(), minimumRate, maximumRate));
  if (ImGui::SliderFloat("Playback Speed",
                         &playbackRate,
                         static_cast<float>(minimumRate),
                         static_cast<float>(maximumRate),
                         "%.2fx")) {
    transport.setPlaybackRate(clampRange(playbackRate, minimumRate, maximumRate));
  }
}

void renderFallingNotesSettings(FallingNotesSettings& settings)
{
  editDoubleSlider("Look Ahead", settings.lookAheadSeconds, 1.0, 30.0);
  editDoubleSlider("Visible Past", settings.visiblePastSeconds, 0.0, 5.0);

  ImGui::SeparatorText("Colors");
  editColor("Note", settings.noteColor);
  editColor("Active Note", settings.activeNoteColor);
}

void renderKeyboardSettings(KeyboardSettings& settings)
{
  editColor("White Key", settings.whiteKeyColor);
  editColor("Black Key", settings.blackKeyColor);
  editColor("Active White Key", settings.activeWhiteKeyColor);
  editColor("Active Black Key", settings.activeBlackKeyColor);
  editColor("White Key Separator", settings.whiteKeySeparatorColor);
  editColor("Hit Line Color", settings.hitLineColor);

  ImGui::SeparatorText("Visibility");
  ImGui::Checkbox("Show Separators", &settings.includeSeparators);
  ImGui::Checkbox("Show Hit Line", &settings.includeHitLine);

  ImGui::SeparatorText("Dimensions");
  editDoubleSlider("White Key Width", settings.whiteKeyWidth, 0.1, 3.0);
  editDoubleSlider("Keyboard Height", settings.whiteKeyHeight, 0.2, 6.0);

  settings.whiteKeyWidth = clampPositive(settings.whiteKeyWidth, KeyboardSettings{}.whiteKeyWidth);
  settings.whiteKeyHeight =
    clampPositive(settings.whiteKeyHeight, KeyboardSettings{}.whiteKeyHeight);

  editDoubleSlider("Black Key Width", settings.blackKeyWidth, 0.05, settings.whiteKeyWidth);
  editDoubleSlider("Black Key Height", settings.blackKeyHeight, 0.1, settings.whiteKeyHeight);
  editSeparatorWidth(settings.separatorWidth);
  editDoubleSlider("Hit Line Height", settings.hitLineHeight, 0.005, 0.25);

  settings.blackKeyWidth = clampRange(settings.blackKeyWidth, 0.000001, settings.whiteKeyWidth);
  settings.blackKeyHeight = clampRange(settings.blackKeyHeight, 0.000001, settings.whiteKeyHeight);
  settings.separatorWidth = std::max(0.0, finiteOr(settings.separatorWidth, 0.0));
  settings.hitLineHeight = clampPositive(settings.hitLineHeight, KeyboardSettings{}.hitLineHeight);
}

void renderRendererSettings(RendererSettings& settings)
{
  editColor("Clear Color", settings.clearColor);
}

} // namespace

void VisualizationSettingsPanel::render(AppSettings& settings, PlaybackTransport& transport)
{
  if (!ImGui::Begin("Visualization Settings")) {
    ImGui::End();
    return;
  }

  if (ImGui::CollapsingHeader("Playback", ImGuiTreeNodeFlags_DefaultOpen)) {
    renderPlaybackSettings(settings, transport);
  }

  if (ImGui::CollapsingHeader("Falling Notes", ImGuiTreeNodeFlags_DefaultOpen)) {
    renderFallingNotesSettings(settings.fallingNotes);
  }

  if (ImGui::CollapsingHeader("Keyboard", ImGuiTreeNodeFlags_DefaultOpen)) {
    renderKeyboardSettings(settings.keyboard);
  }

  if (ImGui::CollapsingHeader("Renderer", ImGuiTreeNodeFlags_DefaultOpen)) {
    renderRendererSettings(settings.renderer);
  }

  ImGui::End();
}
