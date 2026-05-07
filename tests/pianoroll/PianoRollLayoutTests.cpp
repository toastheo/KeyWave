#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <vector>

#include "pianoroll/PianoRollLayout.hpp"

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

TEST_CASE("PianoRollLayout maps queried notes into normalized viewport coordinates",
          "[pianoroll][layout]")
{
  const std::vector queriedNotes{
      makeQueriedNote(60, -1.0, 3.0),
      makeQueriedNote(62, 3.0, 3.0),
  };

  const auto result =
      PianoRollLayout::build(queriedNotes,
                             PianoRollLayoutViewport{
                                 .timeRange = TimeRange{.startSeconds = 0.0, .endSeconds = 4.0},
                                 .pitchRange = PitchRange{.minPitch = 60, .maxPitch = 62},
                             },
                             PianoRollLayoutConfig{.noteHeight = 1.0, .noteVerticalGap = 0.25});

  REQUIRE(result.notes.size() == 2);
  CHECK(result.pitchLaneCount == 3);
  CHECK(result.contentWidth == Catch::Approx(1.0));
  CHECK(result.contentHeight == Catch::Approx(3.0));
  CHECK_FALSE(result.empty());

  CHECK(result.notes[0].note.pitch == 60);
  CHECK(result.notes[0].x == Catch::Approx(0.0));
  CHECK(result.notes[0].y == Catch::Approx(0.0));
  CHECK(result.notes[0].width == Catch::Approx(0.5));
  CHECK(result.notes[0].height == Catch::Approx(0.75));
  CHECK(result.notes[0].visibleStartSeconds == Catch::Approx(0.0));
  CHECK(result.notes[0].visibleEndSeconds == Catch::Approx(2.0));
  CHECK(result.notes[0].clippedLeft);
  CHECK_FALSE(result.notes[0].clippedRight);

  CHECK(result.notes[1].note.pitch == 62);
  CHECK(result.notes[1].x == Catch::Approx(0.75));
  CHECK(result.notes[1].y == Catch::Approx(2.0));
  CHECK(result.notes[1].width == Catch::Approx(0.25));
  CHECK(result.notes[1].height == Catch::Approx(0.75));
  CHECK(result.notes[1].visibleStartSeconds == Catch::Approx(3.0));
  CHECK(result.notes[1].visibleEndSeconds == Catch::Approx(4.0));
  CHECK_FALSE(result.notes[1].clippedLeft);
  CHECK(result.notes[1].clippedRight);
}

TEST_CASE("PianoRollLayout skips notes outside the clipped visible duration", "[pianoroll][layout]")
{
  const std::vector queriedNotes{
      makeQueriedNote(60, -2.0, 1.0),
      makeQueriedNote(61, 1.0, 1.0),
      makeQueriedNote(62, 3.0, 1.0),
  };

  const auto result =
      PianoRollLayout::build(queriedNotes,
                             PianoRollLayoutViewport{
                                 .timeRange = TimeRange{.startSeconds = 0.0, .endSeconds = 3.0},
                                 .pitchRange = PitchRange{.minPitch = 60, .maxPitch = 62},
                             });

  REQUIRE(result.notes.size() == 1);
  CHECK(result.notes[0].note.pitch == 61);
}

TEST_CASE("PianoRollLayout clamps invalid vertical gaps without removing notes",
          "[pianoroll][layout]")
{
  const std::vector queriedNotes{
      makeQueriedNote(60, 0.0, 1.0),
  };

  const auto result =
      PianoRollLayout::build(queriedNotes,
                             PianoRollLayoutViewport{
                                 .timeRange = TimeRange{.startSeconds = 0.0, .endSeconds = 2.0},
                                 .pitchRange = PitchRange{.minPitch = 60, .maxPitch = 60},
                             },
                             PianoRollLayoutConfig{.noteHeight = 1.0, .noteVerticalGap = 5.0});

  REQUIRE(result.notes.size() == 1);
  CHECK(result.notes[0].height > 0.0);
  CHECK(result.notes[0].height <= 1.0);
}

TEST_CASE("PianoRollLayout returns empty results for invalid viewports", "[pianoroll][layout]")
{
  const std::vector queriedNotes{
      makeQueriedNote(60, 0.0, 1.0),
  };

  constexpr PianoRollLayout layout;

  CHECK(layout
            .build(queriedNotes,
                   PianoRollLayoutViewport{
                       .timeRange = TimeRange{.startSeconds = 1.0, .endSeconds = 1.0},
                       .pitchRange = PitchRange{.minPitch = 60, .maxPitch = 60},
                   })
            .empty());

  CHECK(layout
            .build(queriedNotes,
                   PianoRollLayoutViewport{
                       .timeRange = TimeRange{.startSeconds = 0.0, .endSeconds = 1.0},
                       .pitchRange = PitchRange{.minPitch = 61, .maxPitch = 60},
                   })
            .empty());
}

} // namespace
