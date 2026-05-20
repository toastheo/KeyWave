#pragma once

#include "app/AppSettings.hpp"
#include "playback/PlaybackTransport.hpp"

class VisualizationSettingsPanel
{
public:
  static void render(AppSettings& settings, PlaybackTransport& transport);
};
