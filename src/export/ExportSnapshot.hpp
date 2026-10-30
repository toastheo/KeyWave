#pragma once

#include "core/CoreTypes.hpp"
#include "fallingnotes/PianoRollSceneBuilder.hpp"
#include "midi/MidiTimeline.hpp"

// Owns a deep copy of the timeline and visualization; no live controller or transport references.
// Invalid source data throws std::invalid_argument.
struct ExportSnapshot
{
  ExportSnapshot(MidiTimeline timeline,
                 PianoRollSceneConfig sceneConfig,
                 Color background,
                 double timelineOffsetSeconds,
                 double playbackRate);

  // Const members copy even for rvalues; keep those potentially throwing copy
  // semantics explicit rather than generating a misleading move constructor.
  ExportSnapshot(const ExportSnapshot&) = default;

  const MidiTimeline timeline;
  const PianoRollSceneConfig sceneConfig;
  const Color background;
  const double timelineOffsetSeconds;
  const double playbackRate;

  [[nodiscard]] double playbackDurationSeconds() const;
};
