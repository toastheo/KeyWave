#include "ui/VisualizationSettingsPanel.hpp"

#include <algorithm>
#include <array>
#include <cfloat>
#include <cmath>
#include <imgui.h>
#include <span>
#include <string>
#include <string_view>

#include "app/AppSettings.hpp"
#include "app/AppSettingsConstraints.hpp"
#include "app/MidiLibraryStore.hpp"
#include "core/CoreTypes.hpp"
#include "playback/PlaybackTransport.hpp"

namespace {

constexpr float importedMidiPopupMinWidth = 320.0f;

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

template <typename Body> void disabledIf(const bool disabled, Body body)
{
  if (disabled) {
    ImGui::BeginDisabled();
  }
  body();
  if (disabled) {
    ImGui::EndDisabled();
  }
}

void renderPlaybackSettings(AppSettings const& settings,
                            PlaybackTransport& transport,
                            const double sourceBpm)
{
  const auto playbackSettings = sanitizePlaybackControlSettings(settings.playbackControls);
  const auto minimumBpm = playbackSettings.minPlaybackBpm;
  const auto maximumBpm = playbackSettings.maxPlaybackBpm;

  auto playbackBpm = static_cast<int>(
    std::round(clampRange(transport.effectiveBpm(sourceBpm), minimumBpm, maximumBpm)));
  const auto minimumBpmValue = static_cast<int>(std::round(minimumBpm));
  const auto maximumBpmValue = static_cast<int>(std::round(maximumBpm));
  if (ImGui::SliderInt("BPM", &playbackBpm, minimumBpmValue, maximumBpmValue)) {
    transport.setEffectiveBpm(sourceBpm, clampRange(playbackBpm, minimumBpm, maximumBpm));
  }
}

VisualizationSettingsPanelResult renderImportedMidiList(
  const std::span<const ImportedMidiFile> importedMidiFiles,
  const std::string_view activeImportedMidiId)
{
  VisualizationSettingsPanelResult result;

  // The modal edit has to live outside the loop body because ImGui rebuilds this UI every frame.
  static std::string renamingImportedMidiId;
  static std::string removingImportedMidiId;
  static std::string removingImportedMidiName;
  static std::array<char, 256> renameBuffer{};

  ImGui::SeparatorText("Imported MIDI");
  if (importedMidiFiles.empty()) {
    ImGui::TextUnformatted("No imported MIDI files.");
    return result;
  }

  for (const auto& file : importedMidiFiles) {
    ImGui::PushID(file.id.c_str());
    const bool selected = file.id == activeImportedMidiId;
    const auto renameButtonWidth =
      ImGui::CalcTextSize("Rename").x + (ImGui::GetStyle().FramePadding.x * 2.0f);
    const auto removeButtonWidth =
      ImGui::CalcTextSize("Remove").x + (ImGui::GetStyle().FramePadding.x * 2.0f);
    const auto selectableWidth =
      std::max(1.0f,
               ImGui::GetContentRegionAvail().x - renameButtonWidth - removeButtonWidth -
                 ImGui::GetStyle().ItemSpacing.x * 2.0f);
    if (ImGui::Selectable(file.displayName.c_str(), selected, 0, ImVec2(selectableWidth, 0.0f))) {
      result.action = VisualizationSettingsPanelAction::LoadImportedMidiFile;
      result.selectedImportedMidiId = file.id;
    }
    if (selected) {
      ImGui::SetItemDefaultFocus();
    }

    ImGui::SameLine();
    if (ImGui::SmallButton("Rename")) {
      renamingImportedMidiId = file.id;
      std::ranges::fill(renameBuffer, '\0');
      const auto copyLength = std::min(file.displayName.size(), renameBuffer.size() - 1);
      std::copy_n(file.displayName.data(), copyLength, renameBuffer.data());
      ImGui::OpenPopup("Rename Imported MIDI");
    }

    ImGui::SetNextWindowSizeConstraints(ImVec2(importedMidiPopupMinWidth, 0.0f),
                                        ImVec2(FLT_MAX, FLT_MAX));
    if (ImGui::BeginPopupModal("Rename Imported MIDI", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
      ImGui::InputText("Display Name", renameBuffer.data(), renameBuffer.size());
      if (ImGui::Button("Save")) {
        result.action = VisualizationSettingsPanelAction::RenameImportedMidiFile;
        result.selectedImportedMidiId = renamingImportedMidiId;
        result.renamedImportedMidiDisplayName = renameBuffer.data();
        ImGui::CloseCurrentPopup();
      }
      ImGui::SameLine();
      if (ImGui::Button("Cancel")) {
        ImGui::CloseCurrentPopup();
      }
      ImGui::EndPopup();
    }

    ImGui::SameLine();
    if (ImGui::SmallButton("Remove")) {
      removingImportedMidiId = file.id;
      removingImportedMidiName = file.displayName;
      ImGui::OpenPopup("Remove Imported MIDI");
    }

    ImGui::SetNextWindowSizeConstraints(ImVec2(importedMidiPopupMinWidth, 0.0f),
                                        ImVec2(FLT_MAX, FLT_MAX));
    if (ImGui::BeginPopupModal("Remove Imported MIDI", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
      ImGui::TextWrapped("Remove imported MIDI \"%s\"?", removingImportedMidiName.c_str());
      if (ImGui::Button("Remove")) {
        result.action = VisualizationSettingsPanelAction::RemoveImportedMidiFile;
        result.selectedImportedMidiId = removingImportedMidiId;
        ImGui::CloseCurrentPopup();
      }
      ImGui::SameLine();
      if (ImGui::Button("Cancel")) {
        ImGui::CloseCurrentPopup();
      }
      ImGui::EndPopup();
    }

    ImGui::PopID();
  }

  return result;
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
    std::max(constraints.whiteKeyWidth.minimum,
             finiteOr(settings.whiteKeyWidth, KeyboardSettings{}.whiteKeyWidth));
  settings.whiteKeyHeight =
    std::max(constraints.whiteKeyHeight.minimum,
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
    clampRange(settings.blackKeyWidth, constraints.blackKeyWidth.minimum, settings.whiteKeyWidth);
  settings.blackKeyHeight = clampRange(settings.blackKeyHeight,
                                       constraints.blackKeyHeight.minimum,
                                       settings.whiteKeyHeight);
  settings.separatorWidth =
    std::max(constraints.separatorWidth.minimum, finiteOr(settings.separatorWidth, 0.0));
  settings.hitLineHeight =
    std::max(constraints.hitLineHeight.minimum,
             finiteOr(settings.hitLineHeight, KeyboardSettings{}.hitLineHeight));
}

void renderRendererSettings(RendererSettings& settings)
{
  editColor("Clear Color", settings.clearColor);
}

void renderWindowSettings(WindowSettings& settings)
{
  // TODO: Fix switching display modes at runtime on linux. This is bugged for some reason.
#if !defined(__linux__)
  const auto displayMode = settings.displayMode;
  if (const char* displayModeLabel = windowDisplayModeLabel(displayMode);
      ImGui::BeginCombo("Display Mode", displayModeLabel)) {
    for (const auto mode : {WindowDisplayMode::Windowed,
                            WindowDisplayMode::BorderlessFullscreen,
                            WindowDisplayMode::ExclusiveFullscreen}) {
      const bool selected = mode == displayMode;
      if (ImGui::Selectable(windowDisplayModeLabel(mode), selected)) {
        settings.displayMode = mode;
      }
      if (selected) {
        ImGui::SetItemDefaultFocus();
      }
    }
    ImGui::EndCombo();
  }
#endif

  const auto resolutions = windowResolutionPresets();
  const auto currentResolution =
    std::ranges::find_if(resolutions, [&settings](const WindowResolutionPreset& preset) {
      return preset.width == settings.width && preset.height == settings.height;
    });
  const char* resolutionLabel = currentResolution != resolutions.end() ? currentResolution->label
                                                                       : "1280 x 720 (16:9)";

  disabledIf(settings.displayMode != WindowDisplayMode::Windowed,
             [&settings, resolutions, resolutionLabel] {
               if (ImGui::BeginCombo("Resolution", resolutionLabel)) {
                 for (const auto& preset : resolutions) {
                   const bool selected = settings.width == preset.width &&
                                         settings.height == preset.height;
                   if (ImGui::Selectable(preset.label, selected)) {
                     settings.width = preset.width;
                     settings.height = preset.height;
                   }
                   if (selected) {
                     ImGui::SetItemDefaultFocus();
                   }
                 }
                 ImGui::EndCombo();
               }
             });

  ImGui::Checkbox("Vsync", &settings.vsyncEnabled);

  const auto fpsLimits = windowFpsLimitPresets();
  const auto currentLimit =
    std::ranges::find_if(fpsLimits, [&settings](const WindowFpsLimitPreset& preset) {
      return preset.fpsLimit == settings.fpsLimit;
    });
  const char* fpsLimitLabel = currentLimit != fpsLimits.end() ? currentLimit->label : "60";

  disabledIf(settings.vsyncEnabled, [&settings, fpsLimits, fpsLimitLabel] {
    if (ImGui::BeginCombo("FPS Limit", fpsLimitLabel)) {
      for (const auto& preset : fpsLimits) {
        const bool selected = settings.fpsLimit == preset.fpsLimit;
        if (ImGui::Selectable(preset.label, selected)) {
          settings.fpsLimit = preset.fpsLimit;
        }
        if (selected) {
          ImGui::SetItemDefaultFocus();
        }
      }
      ImGui::EndCombo();
    }
  });
}

} // namespace

VisualizationSettingsPanelResult VisualizationSettingsPanel::render(
  AppSettings& settings,
  PlaybackTransport& transport,
  const std::span<const ImportedMidiFile> importedMidiFiles,
  const std::string_view activeImportedMidiId,
  const double sourceBpm)
{
  if (!ImGui::Begin("Visualization Settings")) {
    ImGui::End();
    return {};
  }

  VisualizationSettingsPanelResult result;

  if (ImGui::Button("Reset Settings")) {
    resetAppSettingsToDefaults(settings);
  }

  if (ImGui::CollapsingHeader("Window", ImGuiTreeNodeFlags_DefaultOpen)) {
    renderWindowSettings(settings.window);
  }

  if (ImGui::CollapsingHeader("Playback", ImGuiTreeNodeFlags_DefaultOpen)) {
    renderPlaybackSettings(settings, transport, sourceBpm);
    if (ImGui::Button("Load MIDI...")) {
      result.action = VisualizationSettingsPanelAction::LoadMidiFile;
    }
    if (const auto listResult = renderImportedMidiList(importedMidiFiles, activeImportedMidiId);
        listResult.action != VisualizationSettingsPanelAction::None) {
      result = listResult;
    }
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
  return result;
}
