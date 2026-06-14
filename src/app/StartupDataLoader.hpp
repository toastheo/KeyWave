#pragma once

#include <optional>

#include "app/AppConfig.hpp"
#include "app/MidiLibraryStore.hpp"
#include "diagnostics/Diagnostics.hpp"
#include "midi/MidiTimeline.hpp"

struct StartupData
{
  std::optional<MidiTimeline> timeline;
};

class StartupDataLoader
{
public:
  [[nodiscard]] static StartupData load(const AppConfig& config,
                                        DiagnosticSink& diagnostics = nullDiagnosticSink());
  [[nodiscard]] static StartupData load(const AppConfig& config,
                                        const MidiLibraryStore& midiLibraryStore,
                                        DiagnosticSink& diagnostics = nullDiagnosticSink());
};
