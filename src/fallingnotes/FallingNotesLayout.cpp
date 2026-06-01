#include "fallingnotes/FallingNotesLayout.hpp"

#include <algorithm>
#include <cmath>

namespace {

constexpr double kMinimumPositiveWidth = 0.000001;

bool isValidPitchRange(const PitchRange& range)
{
  return range.minPitch <= range.maxPitch;
}

bool isValidViewport(const FallingNotesViewport& viewport)
{
  return isValidPitchRange(viewport.pitchRange) && std::isfinite(viewport.currentTimeSeconds) &&
         std::isfinite(viewport.lookAheadSeconds) && viewport.lookAheadSeconds > 0.0 &&
         std::isfinite(viewport.visiblePastSeconds) && viewport.visiblePastSeconds >= 0.0 &&
         std::isfinite(viewport.displayHeight);
}

double noteEndSeconds(const Note& note)
{
  return note.startSeconds + note.durationSeconds;
}

bool hasPositiveWidth(const Rect& rect)
{
  return std::isfinite(rect.x) && std::isfinite(rect.width) && rect.width > 0.0;
}

double displayHeightFor(const FallingNotesViewport& viewport)
{
  return viewport.displayHeight > 0.0 ? viewport.displayHeight : viewport.lookAheadSeconds;
}

double nonNegativeFiniteOrZero(const double value)
{
  return std::isfinite(value) && value >= 0.0 ? value : 0.0;
}

double positiveFiniteOrOne(const double value)
{
  return std::isfinite(value) && value > 0.0 ? value : 1.0;
}

FallingNotesLayoutStyle sanitizedStyle(FallingNotesLayoutStyle style)
{
  style.noteHorizontalInset = nonNegativeFiniteOrZero(style.noteHorizontalInset);
  style.blackNoteWidthScale = positiveFiniteOrOne(style.blackNoteWidthScale);
  style.whiteNoteWidthScale = positiveFiniteOrOne(style.whiteNoteWidthScale);
  return style;
}

Rect shapedNoteRect(Rect rect, const int pitch, const FallingNotesLayoutStyle& style)
{
  const auto maximumInset = std::max(0.0, (rect.width - kMinimumPositiveWidth) * 0.5);
  const auto inset = std::min(style.noteHorizontalInset, maximumInset);
  const auto insetWidth = std::max(kMinimumPositiveWidth, rect.width - inset * 2.0);
  const auto widthScale = KeyboardGeometry::isBlackKey(pitch) ? style.blackNoteWidthScale
                                                              : style.whiteNoteWidthScale;
  const auto scaledWidth = std::max(kMinimumPositiveWidth, insetWidth * widthScale);

  rect.x += inset + (insetWidth - scaledWidth) * 0.5;
  rect.width = scaledWidth;
  return rect;
}

} // namespace

bool FallingNotesLayoutResult::empty() const
{
  return notes.empty();
}

FallingNotesLayoutResult FallingNotesLayout::build(const std::vector<QueriedNote>& queriedNotes,
                                                   const FallingNotesViewport& viewport,
                                                   const KeyboardGeometry& geometry,
                                                   const FallingNotesLayoutStyle& style)
{
  FallingNotesLayoutResult result{
    .pitchRange = viewport.pitchRange,
    .currentTimeSeconds = viewport.currentTimeSeconds,
    .lookAheadSeconds = viewport.lookAheadSeconds,
    .visiblePastSeconds = viewport.visiblePastSeconds,
    .displayHeight = displayHeightFor(viewport),
  };

  if (!isValidViewport(viewport)) {
    return result;
  }

  // The vertical axis is measured in seconds relative to currentTimeSeconds:
  // y = 0 is the keyboard hit line, positive values are upcoming notes, negative values are past
  // notes.
  const auto minY = -viewport.visiblePastSeconds;
  const auto maxY = viewport.lookAheadSeconds;
  const auto secondsToWorldScale = result.displayHeight / viewport.lookAheadSeconds;

  result.pitchLaneCount = geometry.whiteKeyCount();
  result.contentWidth = geometry.width();
  result.contentHeight = result.displayHeight;
  result.notes.reserve(queriedNotes.size());
  const auto shapedStyle = sanitizedStyle(style);

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

    if (!geometry.containsPitch(note.pitch)) {
      continue;
    }

    const auto noteRect =
      shapedNoteRect(geometry.noteRectForPitch(note.pitch), note.pitch, shapedStyle);
    if (!hasPositiveWidth(noteRect)) {
      continue;
    }

    result.notes.push_back(FallingNoteLayout{
      .note = note,
      .x = noteRect.x,
      .y = visibleStartOffset * secondsToWorldScale,
      .width = noteRect.width,
      .height = visibleHeight * secondsToWorldScale,
      .visibleStartOffsetSeconds = visibleStartOffset,
      .visibleEndOffsetSeconds = visibleEndOffset,
      .clippedBottom = (noteStartOffset < minY),
      .clippedTop = (noteEndOffset > maxY),
    });
  }

  return result;
}
