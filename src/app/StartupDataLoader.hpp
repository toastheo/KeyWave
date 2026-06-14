#pragma once

#include <optional>
#include <string>

#include "app/AppConfig.hpp"
#include "app/MidiLibraryStore.hpp"
#include "diagnostics/Diagnostics.hpp"
#include "midi/MidiTimeline.hpp"

struct StartupData
{
  std::optional<MidiTimeline> timeline;
  std::optional<std::string> importedMidiId;
};

class StartupDataLoader
{
public:
  [[nodiscard]] static StartupData load(const AppConfig& config,
                                        const MidiLibraryStore& midiLibraryStore,
                                        DiagnosticSink& diagnostics = nullDiagnosticSink());
};
