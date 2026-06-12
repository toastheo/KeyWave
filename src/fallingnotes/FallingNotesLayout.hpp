#pragma once

#include <span>

#include "fallingnotes/FallingNotesTypes.hpp"
#include "keyboard/KeyboardGeometry.hpp"
#include "midi/MidiTimelineQuery.hpp"

class FallingNotesLayout
{
public:
  [[nodiscard]] static FallingNotesLayoutResult build(std::span<const QueriedNote> queriedNotes,
                                                      const FallingNotesViewport& viewport,
                                                      const KeyboardGeometry& geometry,
                                                      const FallingNotesLayoutStyle& style = {});
};
