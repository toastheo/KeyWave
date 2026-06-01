#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <vector>

#include "fallingnotes/FallingNotesLayout.hpp"
#include "keyboard/KeyboardGeometry.hpp"

namespace {

QueriedNote makeQueriedNote(const int pitch,
                            const double startSeconds,
                            const double durationSeconds)
{
  return QueriedNote{
    .note =
      Note{
        .pitch = pitch,
        .startSeconds = startSeconds,
        .durationSeconds = durationSeconds,
      },
  };
}

TEST_CASE("FallingNotesLayout maps keyboard pitch rectangles and time from the hit line",
          "[fallingnotes][layout]")
{
  const std::vector queriedNotes{
    makeQueriedNote(21, 0.0, 1.0),
    makeQueriedNote(23, 2.0, 3.0),
  };

  const auto result =
    FallingNotesLayout::build(queriedNotes,
                              FallingNotesViewport{
                                .pitchRange = PitchRange{.minPitch = 21, .maxPitch = 23},
                                .currentTimeSeconds = 1.0,
                                .lookAheadSeconds = 10.0,
                                .visiblePastSeconds = 2.0,
                              },
                              KeyboardGeometry(KeyboardLayoutConfig{
                                .pitchRange = PitchRange{.minPitch = 21, .maxPitch = 23},
                              }));

  REQUIRE(result.notes.size() == 2);
  CHECK(result.pitchRange.minPitch == 21);
  CHECK(result.pitchRange.maxPitch == 23);
  CHECK(result.currentTimeSeconds == Catch::Approx(1.0));
  CHECK(result.lookAheadSeconds == Catch::Approx(10.0));
  CHECK(result.visiblePastSeconds == Catch::Approx(2.0));
  CHECK(result.pitchLaneCount == 2);
  CHECK(result.contentWidth == Catch::Approx(2.0));
  CHECK(result.contentHeight == Catch::Approx(10.0));
  CHECK_FALSE(result.empty());

  CHECK(result.notes[0].note.pitch == 21);
  CHECK(result.notes[0].x == Catch::Approx(0.0075));
  CHECK(result.notes[0].y == Catch::Approx(-1.0));
  CHECK(result.notes[0].width == Catch::Approx(0.985));
  CHECK(result.notes[0].height == Catch::Approx(1.0));
  CHECK(result.notes[0].visibleStartOffsetSeconds == Catch::Approx(-1.0));
  CHECK(result.notes[0].visibleEndOffsetSeconds == Catch::Approx(0.0));
  CHECK_FALSE(result.notes[0].clippedBottom);
  CHECK_FALSE(result.notes[0].clippedTop);

  CHECK(result.notes[1].note.pitch == 23);
  CHECK(result.notes[1].x == Catch::Approx(1.0075));
  CHECK(result.notes[1].y == Catch::Approx(1.0));
  CHECK(result.notes[1].width == Catch::Approx(0.985));
  CHECK(result.notes[1].height == Catch::Approx(3.0));
  CHECK(result.notes[1].visibleStartOffsetSeconds == Catch::Approx(1.0));
  CHECK(result.notes[1].visibleEndOffsetSeconds == Catch::Approx(4.0));
}

TEST_CASE("FallingNotesLayout clips notes against visible past and lookahead",
          "[fallingnotes][layout]")
{
  const std::vector queriedNotes{
    makeQueriedNote(60, -3.0, 4.0),
    makeQueriedNote(61, 8.0, 5.0),
    makeQueriedNote(62, -5.0, 1.0),
    makeQueriedNote(63, 11.0, 1.0),
  };

  const auto result =
    FallingNotesLayout::build(queriedNotes,
                              FallingNotesViewport{
                                .pitchRange = PitchRange{.minPitch = 60, .maxPitch = 63},
                                .currentTimeSeconds = 0.0,
                                .lookAheadSeconds = 10.0,
                                .visiblePastSeconds = 2.0,
                              },
                              KeyboardGeometry(KeyboardLayoutConfig{
                                .pitchRange = PitchRange{.minPitch = 60, .maxPitch = 63},
                              }));

  REQUIRE(result.notes.size() == 2);

  CHECK(result.notes[0].note.pitch == 60);
  CHECK(result.notes[0].y == Catch::Approx(-2.0));
  CHECK(result.notes[0].height == Catch::Approx(3.0));
  CHECK(result.notes[0].visibleStartOffsetSeconds == Catch::Approx(-2.0));
  CHECK(result.notes[0].visibleEndOffsetSeconds == Catch::Approx(1.0));
  CHECK(result.notes[0].clippedBottom);
  CHECK_FALSE(result.notes[0].clippedTop);

  CHECK(result.notes[1].note.pitch == 61);
  CHECK(result.notes[1].y == Catch::Approx(8.0));
  CHECK(result.notes[1].height == Catch::Approx(2.0));
  CHECK_FALSE(result.notes[1].clippedBottom);
  CHECK(result.notes[1].clippedTop);
}

TEST_CASE("FallingNotesLayout skips pitches outside the viewport range", "[fallingnotes][layout]")
{
  const std::vector queriedNotes{
    makeQueriedNote(59, 1.0, 1.0),
    makeQueriedNote(60, 1.0, 1.0),
    makeQueriedNote(63, 1.0, 1.0),
  };

  const auto result =
    FallingNotesLayout::build(queriedNotes,
                              FallingNotesViewport{
                                .pitchRange = PitchRange{.minPitch = 60, .maxPitch = 62},
                                .currentTimeSeconds = 0.0,
                                .lookAheadSeconds = 4.0,
                                .visiblePastSeconds = 0.0,
                              },
                              KeyboardGeometry(KeyboardLayoutConfig{
                                .pitchRange = PitchRange{.minPitch = 60, .maxPitch = 62},
                              }));

  REQUIRE(result.notes.size() == 1);
  CHECK(result.notes[0].note.pitch == 60);
}

TEST_CASE("FallingNotesLayout can use keyboard geometry for piano-aligned note widths",
          "[fallingnotes][layout]")
{
  const std::vector queriedNotes{
    makeQueriedNote(60, 1.0, 1.0),
    makeQueriedNote(61, 1.5, 1.0),
  };
  const KeyboardGeometry geometry(KeyboardLayoutConfig{
    .pitchRange = PitchRange{.minPitch = 60, .maxPitch = 62},
    .whiteKeyWidth = 2.0,
    .blackKeyWidth = 1.0,
    .whiteKeyGap = 0.2,
  });

  const auto result =
    FallingNotesLayout::build(queriedNotes,
                              FallingNotesViewport{
                                .pitchRange = PitchRange{.minPitch = 60, .maxPitch = 62},
                                .currentTimeSeconds = 0.0,
                                .lookAheadSeconds = 4.0,
                                .visiblePastSeconds = 0.0,
                              },
                              geometry);

  REQUIRE(result.notes.size() == 2);
  CHECK(result.contentWidth == Catch::Approx(geometry.width()));
  CHECK(result.notes[0].note.pitch == 60);
  CHECK(result.notes[0].x == Catch::Approx(0.1));
  CHECK(result.notes[0].width == Catch::Approx(1.8));
  CHECK(result.notes[1].note.pitch == 61);
  CHECK(result.notes[1].x == Catch::Approx(1.5));
  CHECK(result.notes[1].width == Catch::Approx(1.0));
}

TEST_CASE("FallingNotesLayout applies falling-note inset and width scales",
          "[fallingnotes][layout]")
{
  const std::vector queriedNotes{
    makeQueriedNote(60, 1.0, 1.0),
    makeQueriedNote(61, 1.5, 1.0),
  };
  const KeyboardGeometry geometry(KeyboardLayoutConfig{
    .pitchRange = PitchRange{.minPitch = 60, .maxPitch = 62},
    .whiteKeyWidth = 2.0,
    .blackKeyWidth = 1.0,
    .whiteKeyGap = 0.2,
  });

  const auto result =
    FallingNotesLayout::build(queriedNotes,
                              FallingNotesViewport{
                                .pitchRange = PitchRange{.minPitch = 60, .maxPitch = 62},
                                .currentTimeSeconds = 0.0,
                                .lookAheadSeconds = 4.0,
                                .visiblePastSeconds = 0.0,
                              },
                              geometry,
                              FallingNotesLayoutStyle{
                                .noteHorizontalInset = 0.1,
                                .blackNoteWidthScale = 0.75,
                                .whiteNoteWidthScale = 0.5,
                              });

  REQUIRE(result.notes.size() == 2);
  CHECK(result.notes[0].note.pitch == 60);
  CHECK(result.notes[0].x == Catch::Approx(0.6));
  CHECK(result.notes[0].width == Catch::Approx(0.8));
  CHECK(result.notes[1].note.pitch == 61);
  CHECK(result.notes[1].x == Catch::Approx(1.7));
  CHECK(result.notes[1].width == Catch::Approx(0.6));
}

TEST_CASE("FallingNotesLayout returns empty results for invalid viewports",
          "[fallingnotes][layout]")
{
  const std::vector queriedNotes{
    makeQueriedNote(60, 1.0, 1.0),
  };

  CHECK(FallingNotesLayout{}
          .build(queriedNotes,
                 FallingNotesViewport{
                   .pitchRange = PitchRange{.minPitch = 61, .maxPitch = 60},
                   .currentTimeSeconds = 0.0,
                   .lookAheadSeconds = 4.0,
                   .visiblePastSeconds = 0.0,
                 },
                 KeyboardGeometry{})
          .empty());

  CHECK(FallingNotesLayout{}
          .build(queriedNotes,
                 FallingNotesViewport{
                   .pitchRange = PitchRange{.minPitch = 60, .maxPitch = 60},
                   .currentTimeSeconds = 0.0,
                   .lookAheadSeconds = 0.0,
                   .visiblePastSeconds = 0.0,
                 },
                 KeyboardGeometry{})
          .empty());
}

} // namespace
