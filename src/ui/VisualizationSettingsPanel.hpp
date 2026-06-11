#pragma once

#include "app/AppSettings.hpp"
#include "playback/PlaybackTransport.hpp"

enum class VisualizationSettingsPanelAction
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
