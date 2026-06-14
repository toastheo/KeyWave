#pragma once

#include <span>
#include <string>
#include <string_view>

#include "app/AppSettings.hpp"
#include "app/MidiLibraryStore.hpp"
#include "playback/PlaybackTransport.hpp"

enum class VisualizationSettingsPanelAction : std::uint8_t
{
  None,
  LoadMidiFile,
  LoadImportedMidiFile,
};

struct VisualizationSettingsPanelResult
{
  VisualizationSettingsPanelAction action = VisualizationSettingsPanelAction::None;
  std::string selectedImportedMidiId;
};

class VisualizationSettingsPanel
{
public:
  static VisualizationSettingsPanelResult render(AppSettings& settings,
                                                 PlaybackTransport& transport,
                                                 std::span<const ImportedMidiFile> importedMidiFiles,
                                                 std::string_view activeImportedMidiId);
};
