#pragma once

#include <vector>

#include "fallingnotes/FallingNotesTypes.hpp"
#include "keyboard/KeyboardGeometry.hpp"
#include "midi/MidiTimelineQuery.hpp"

class FallingNotesLayout
{
public:
  [[nodiscard]] static FallingNotesLayoutResult build(const std::vector<QueriedNote>& queriedNotes,
                                                      const FallingNotesViewport& viewport,
                                                      const KeyboardGeometry& geometry);
};
