#include <array>
#include <catch2/catch_test_macros.hpp>
#include <span>

#include "fallingnotes/FallingNotesLayout.hpp"
#include "keyboard/KeyboardGeometry.hpp"
#include "keyboard/KeyboardState.hpp"
#include "render/RendererBackend.hpp"

namespace {

class RecordingRendererBackend final : public RendererBackend
{
public:
  bool initialize() override
  {
    return true;
  }
  void shutdown() override {}
  void setFramebufferSize(const FramebufferSize&) override {}
  void setClearColor(Color) override {}
  void setView(const RendererView&) override {}
  void beginFrame() override {}
  void submit(std::span<const RenderCommand> commands) override
  {
    submittedCommandCount = commands.size();
  }
  void endFrame() override {}

  std::size_t submittedCommandCount = 0U;
};

TEST_CASE("render and layout APIs accept contiguous spans", "[architecture]")
{
  const std::array commands{
    RenderCommand{DrawRectCommand{
      .rect = Rect{.x = 0.0, .y = 0.0, .width = 1.0, .height = 1.0},
      .color = Color{.r = 1.0F, .g = 1.0F, .b = 1.0F, .a = 1.0F},
    }},
  };
  RecordingRendererBackend renderer;
  renderer.submit(commands);
  CHECK(renderer.submittedCommandCount == commands.size());

  const std::array queriedNotes{
    QueriedNote{.note = Note{.pitch = 60, .velocity = 100, .durationSeconds = 1.0}},
  };
  const auto layout =
    FallingNotesLayout::build(queriedNotes,
                              FallingNotesViewport{.pitchRange = {.minPitch = 60, .maxPitch = 72}},
                              KeyboardGeometry{});
  CHECK(layout.notes.size() == queriedNotes.size());

  const std::array activeNotes{
    Note{.pitch = 64, .velocity = 90, .channel = 1, .track = 2},
  };
  const auto state = KeyboardStateBuilder::build(activeNotes);
  CHECK(state.isActive(activeNotes.front().pitch));
}

} // namespace
