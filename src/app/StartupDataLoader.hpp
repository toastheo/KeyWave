#pragma once

#include <optional>

#include "app/AppConfig.hpp"
#include "midi/MidiTimeline.hpp"

struct StartupData
{
  std::optional<MidiTimeline> timeline;
};

class StartupDataLoader
{
public:
  [[nodiscard]] static StartupData load(const AppConfig& config);
};
