#include "ui/VisualizationSettingsPanel.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <imgui.h>

#include "app/AppSettingsConstraints.hpp"

namespace {

double finiteOr(const double value, const double fallback)
{
  return std::isfinite(value) ? value : fallback;
}

double clampRange(const double value, const double minimum, const double maximum)
{
  return std::clamp(finiteOr(value, minimum), minimum, maximum);
}

bool editDoubleSlider(const char* label, double& value, const double minimum, const double maximum)
{
  auto editableValue = static_cast<float>(clampRange(value, minimum, maximum));
  const auto changed = ImGui::SliderFloat(
    label, &editableValue, static_cast<float>(minimum), static_cast<float>(maximum));
  value = clampRange(editableValue, minimum, maximum);
  return changed;
}

bool editDoubleSlider(const char* label, double& value, const DoubleSettingRange range)
{
  return editDoubleSlider(label, value, range.minimum, range.maximum);
}

bool editSeparatorWidth(double& value, const DoubleSettingRange range)
{
  auto editableValue = static_cast<float>(clampRange(value, range.minimum, range.maximum));
  const auto changed = ImGui::SliderFloat("Separator Width",
                                          &editableValue,
                                          static_cast<float>(range.minimum),
                                          static_cast<float>(range.maximum));
  value = clampRange(editableValue, range.minimum, range.maximum);
  return changed;
}

bool editOutlineThickness(double& value, const DoubleSettingRange range)
{
  auto editableValue = static_cast<float>(clampRange(value, range.minimum, range.maximum));
  const auto changed = ImGui::SliderFloat("Outline Thickness",
                                          &editableValue,
                                          static_cast<float>(range.minimum),
                                          static_cast<float>(range.maximum));
  value = clampRange(editableValue, range.minimum, range.maximum);
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

  auto playbackRate =
    static_cast<float>(clampRange(transport.playbackRate(), minimumRate, maximumRate));
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
  const auto constraints = appSettingsConstraints().fallingNotes;
  editDoubleSlider("Look Ahead", settings.lookAheadSeconds, constraints.lookAheadSeconds);
  editDoubleSlider("Visible Past", settings.visiblePastSeconds, constraints.visiblePastSeconds);

  ImGui::SeparatorText("Dimensions");
  editDoubleSlider("Horizontal Inset",
                   settings.noteHorizontalInset,
                   constraints.noteHorizontalInset);
  editDoubleSlider("Black Note Width", settings.blackNoteWidthScale, constraints.noteWidthScale);
  editDoubleSlider("White Note Width", settings.whiteNoteWidthScale, constraints.noteWidthScale);
  editDoubleSlider("Corner Radius", settings.cornerRadiusPixels, constraints.cornerRadiusPixels);

  ImGui::SeparatorText("Colors");
  editColor("Note", settings.noteColor);
  editColor("Active Note", settings.activeNoteColor);

  ImGui::SeparatorText("Outline");
  ImGui::Checkbox("Show Outline", &settings.includeOutline);
  editOutlineThickness(settings.outlineThicknessPixels, constraints.outlineThicknessPixels);
  editColor("Outline Color", settings.outlineColor);
}

void renderKeyboardSettings(KeyboardSettings& settings)
{
  const auto constraints = appSettingsConstraints().keyboard;

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
  editDoubleSlider("White Key Width", settings.whiteKeyWidth, constraints.whiteKeyWidth);
  editDoubleSlider("Keyboard Height", settings.whiteKeyHeight, constraints.whiteKeyHeight);

  // Clamp parent dimensions before editing dependent black-key dimensions, since their maxima
  // are dynamic.
  settings.whiteKeyWidth =
    std::max(constraints.minimumPositiveValue,
             finiteOr(settings.whiteKeyWidth, KeyboardSettings{}.whiteKeyWidth));
  settings.whiteKeyHeight =
    std::max(constraints.minimumPositiveValue,
             finiteOr(settings.whiteKeyHeight, KeyboardSettings{}.whiteKeyHeight));

  editDoubleSlider("Black Key Width",
                   settings.blackKeyWidth,
                   constraints.blackKeyWidth.minimum,
                   settings.whiteKeyWidth);
  editDoubleSlider("Black Key Height",
                   settings.blackKeyHeight,
                   constraints.blackKeyHeight.minimum,
                   settings.whiteKeyHeight);
  editSeparatorWidth(settings.separatorWidth, constraints.separatorWidth);
  editDoubleSlider("Hit Line Height", settings.hitLineHeight, constraints.hitLineHeight);

  settings.blackKeyWidth =
    clampRange(settings.blackKeyWidth, constraints.minimumPositiveValue, settings.whiteKeyWidth);
  settings.blackKeyHeight =
    clampRange(settings.blackKeyHeight, constraints.minimumPositiveValue, settings.whiteKeyHeight);
  settings.separatorWidth =
    std::max(constraints.separatorWidth.minimum, finiteOr(settings.separatorWidth, 0.0));
  settings.hitLineHeight =
    std::max(constraints.minimumPositiveValue,
             finiteOr(settings.hitLineHeight, KeyboardSettings{}.hitLineHeight));
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

  if (ImGui::Button("Reset Settings")) {
    resetAppSettingsToDefaults(settings);
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
