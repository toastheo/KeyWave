#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <vector>

#include "fallingnotes/FallingNotesLayout.hpp"

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

TEST_CASE("FallingNotesLayout maps pitch horizontally and time vertically from the hit line",
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
                              FallingNotesLayoutConfig{
                                .laneWidth = 2.0,
                                .noteHorizontalGap = 0.25,
                              });

  REQUIRE(result.notes.size() == 2);
  CHECK(result.pitchRange.minPitch == 21);
  CHECK(result.pitchRange.maxPitch == 23);
  CHECK(result.currentTimeSeconds == Catch::Approx(1.0));
  CHECK(result.lookAheadSeconds == Catch::Approx(10.0));
  CHECK(result.visiblePastSeconds == Catch::Approx(2.0));
  CHECK(result.pitchLaneCount == 3);
  CHECK(result.contentWidth == Catch::Approx(6.0));
  CHECK(result.contentHeight == Catch::Approx(10.0));
  CHECK_FALSE(result.empty());

  CHECK(result.notes[0].note.pitch == 21);
  CHECK(result.notes[0].x == Catch::Approx(0.0));
  CHECK(result.notes[0].y == Catch::Approx(-1.0));
  CHECK(result.notes[0].width == Catch::Approx(1.75));
  CHECK(result.notes[0].height == Catch::Approx(1.0));
  CHECK(result.notes[0].visibleStartOffsetSeconds == Catch::Approx(-1.0));
  CHECK(result.notes[0].visibleEndOffsetSeconds == Catch::Approx(0.0));
  CHECK_FALSE(result.notes[0].clippedBottom);
  CHECK_FALSE(result.notes[0].clippedTop);

  CHECK(result.notes[1].note.pitch == 23);
  CHECK(result.notes[1].x == Catch::Approx(4.0));
  CHECK(result.notes[1].y == Catch::Approx(1.0));
  CHECK(result.notes[1].width == Catch::Approx(1.75));
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
                              });

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
    makeQueriedNote(62, 1.0, 1.0),
  };

  const auto result =
    FallingNotesLayout::build(queriedNotes,
                              FallingNotesViewport{
                                .pitchRange = PitchRange{.minPitch = 60, .maxPitch = 61},
                                .currentTimeSeconds = 0.0,
                                .lookAheadSeconds = 4.0,
                                .visiblePastSeconds = 0.0,
                              });

  REQUIRE(result.notes.size() == 1);
  CHECK(result.notes[0].note.pitch == 60);
}

TEST_CASE("FallingNotesLayout clamps horizontal gaps so notes keep positive width",
          "[fallingnotes][layout]")
{
  const std::vector queriedNotes{
    makeQueriedNote(60, 1.0, 1.0),
  };

  const auto result =
    FallingNotesLayout::build(queriedNotes,
                              FallingNotesViewport{
                                .pitchRange = PitchRange{.minPitch = 60, .maxPitch = 60},
                                .currentTimeSeconds = 0.0,
                                .lookAheadSeconds = 4.0,
                                .visiblePastSeconds = 0.0,
                              },
                              FallingNotesLayoutConfig{
                                .laneWidth = 1.0,
                                .noteHorizontalGap = 10.0,
                              });

  REQUIRE(result.notes.size() == 1);
  CHECK(result.notes[0].width > 0.0);
  CHECK(result.notes[0].width <= 1.0);
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
                 })
          .empty());

  CHECK(FallingNotesLayout{}
          .build(queriedNotes,
                 FallingNotesViewport{
                   .pitchRange = PitchRange{.minPitch = 60, .maxPitch = 60},
                   .currentTimeSeconds = 0.0,
                   .lookAheadSeconds = 0.0,
                   .visiblePastSeconds = 0.0,
                 })
          .empty());
}

} // namespace
