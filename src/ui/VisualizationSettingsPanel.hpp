#pragma once

#include "app/AppSettings.hpp"
#include "playback/PlaybackTransport.hpp"

enum class VisualizationSettingsPanelAction : std::uint8_t
{
  None,
  LoadMidiFile,
};

class VisualizationSettingsPanel
{
public:
  static VisualizationSettingsPanelAction render(AppSettings& settings,
                                                 PlaybackTransport& transport);
};
