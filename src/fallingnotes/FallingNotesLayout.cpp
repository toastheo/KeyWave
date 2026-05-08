#include "fallingnotes/FallingNotesLayout.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

namespace {

constexpr double kMinimumVisibleNoteWidth = 0.000001;

bool isValidPitchRange(const PitchRange& range)
{
  return range.minPitch <= range.maxPitch;
}

bool isValidViewport(const FallingNotesViewport& viewport)
{
  return isValidPitchRange(viewport.pitchRange) && std::isfinite(viewport.currentTimeSeconds) &&
         std::isfinite(viewport.lookAheadSeconds) && viewport.lookAheadSeconds > 0.0 &&
         std::isfinite(viewport.visiblePastSeconds) && viewport.visiblePastSeconds >= 0.0;
}

int pitchLaneCountForRange(const PitchRange& range)
{
  const auto laneCount =
    static_cast<long long>(range.maxPitch) - static_cast<long long>(range.minPitch) + 1;
  if (laneCount <= 0 || laneCount > std::numeric_limits<int>::max()) {
    return 0;
  }

  return static_cast<int>(laneCount);
}

double sanitizedLaneWidth(const FallingNotesLayoutConfig& config)
{
  if (!std::isfinite(config.laneWidth) || config.laneWidth <= 0.0) {
    return kMinimumVisibleNoteWidth;
  }

  return config.laneWidth;
}

double noteLayoutWidth(const FallingNotesLayoutConfig& config, const double laneWidth)
{
  const auto gap = std::isfinite(config.noteHorizontalGap) ? std::max(0.0, config.noteHorizontalGap)
                                                           : 0.0;
  return std::max(kMinimumVisibleNoteWidth, laneWidth - gap);
}

double noteEndSeconds(const Note& note)
{
  return note.startSeconds + note.durationSeconds;
}

} // namespace

bool FallingNotesLayoutResult::empty() const
{
  return notes.empty();
}

FallingNotesLayoutResult FallingNotesLayout::build(const std::vector<QueriedNote>& queriedNotes,
                                                   const FallingNotesViewport& viewport,
                                                   const FallingNotesLayoutConfig& config)
{
  FallingNotesLayoutResult result{
    .pitchRange = viewport.pitchRange,
    .currentTimeSeconds = viewport.currentTimeSeconds,
    .lookAheadSeconds = viewport.lookAheadSeconds,
    .visiblePastSeconds = viewport.visiblePastSeconds,
  };

  if (!isValidViewport(viewport)) {
    return result;
  }

  const auto pitchLaneCount = pitchLaneCountForRange(viewport.pitchRange);
  if (pitchLaneCount <= 0) {
    return result;
  }

  const auto laneWidth = sanitizedLaneWidth(config);
  const auto noteWidth = noteLayoutWidth(config, laneWidth);
  const auto minY = -viewport.visiblePastSeconds;
  const auto maxY = viewport.lookAheadSeconds;

  result.pitchLaneCount = pitchLaneCount;
  result.contentWidth = static_cast<double>(pitchLaneCount) * laneWidth;
  result.contentHeight = viewport.lookAheadSeconds;
  result.notes.reserve(queriedNotes.size());

  for (const auto& queriedNote : queriedNotes) {
    const auto& note = queriedNote.note;
    const auto noteEnd = noteEndSeconds(note);
    if (!std::isfinite(note.startSeconds) || !std::isfinite(note.durationSeconds) ||
        !std::isfinite(noteEnd)) {
      continue;
    }

    const auto noteStartOffset = note.startSeconds - viewport.currentTimeSeconds;
    const auto noteEndOffset = noteEnd - viewport.currentTimeSeconds;
    if (!(noteEndOffset > minY && noteStartOffset < maxY)) {
      continue;
    }

    const auto visibleStartOffset = std::max(noteStartOffset, minY);
    const auto visibleEndOffset = std::min(noteEndOffset, maxY);
    const auto visibleHeight = visibleEndOffset - visibleStartOffset;
    if (visibleHeight <= 0.0) {
      continue;
    }

    const auto pitchLane = note.pitch - viewport.pitchRange.minPitch;
    if (pitchLane < 0 || pitchLane >= pitchLaneCount) {
      continue;
    }

    result.notes.push_back(FallingNoteLayout{
      .note = note,
      .x = static_cast<double>(pitchLane) * laneWidth,
      .y = visibleStartOffset,
      .width = noteWidth,
      .height = visibleHeight,
      .visibleStartOffsetSeconds = visibleStartOffset,
      .visibleEndOffsetSeconds = visibleEndOffset,
      .clippedBottom = (noteStartOffset < minY),
      .clippedTop = (noteEndOffset > maxY),
    });
  }

  return result;
}
