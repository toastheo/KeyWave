#pragma once

#include <optional>

#include "app/AppConfig.hpp"
#include "midi/MidiTimeline.hpp"

struct StartupData
{
  std::optional<MidiTimeline> timeline;
};

class StartupSceneBuilder
{
public:
  [[nodiscard]] static StartupData load(const AppConfig& config);
};
