#pragma once

#include <vector>

#include "midi/MidiTimelineQuery.hpp"
#include "pianoroll/PianoRollTypes.hpp"

class PianoRollLayout
{
public:
  [[nodiscard]] static PianoRollLayoutResult build(const std::vector<QueriedNote>& queriedNotes,
                                                   const PianoRollLayoutViewport& viewport,
                                                   const PianoRollLayoutConfig& config = {});
};
