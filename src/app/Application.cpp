#include "app/Application.hpp"

#include "midi/MidiFileLoader.hpp"
#include "midi/MidiTimelineQuery.hpp"
#include "render/RenderTypes.hpp"
#include "render_opengl/OpenGLRendererBackend.hpp"

#include <algorithm>
#include <filesystem>
#include <iostream>
#include <memory>

namespace {

std::filesystem::path testMidiPath() {
#ifdef KEYWAVE_SOURCE_DIR
  return std::filesystem::path(KEYWAVE_SOURCE_DIR) / "assets" / "test-midi" / "test.mid";
#else
  return std::filesystem::path("assets") / "test-midi" / "test.mid";
#endif
}

void loadStartupMidiIfPresent() {
  const auto midiPath = testMidiPath();
  if (!std::filesystem::exists(midiPath)) {
    std::cout << "No test MIDI file found at " << midiPath.string()
              << ". Place a .mid file there to test MIDI loading.\n";
    return;
  }
  
  const auto timeline = MidiFileLoader::loadFromFile(midiPath);
  if (!timeline.has_value()) {
    return;
  }

  const MidiTimelineQuery query(*timeline);
  const auto notes = query.findNotes(TimelineViewport{
    .timeRange = TimeRange{.startSeconds = 0.0, .endSeconds = 10.0},
    .pitchRange = PitchRange{.minPitch = 21, .maxPitch = 108},
  });

  std::cout << "MIDI startup query\n";
  std::cout << "  queried notes: " << notes.size() << '\n';

  const auto previewCount = std::min<std::size_t>(notes.size(), 5);
  for (std::size_t index = 0; index < previewCount; ++index) {
    const auto& queriedNote = notes[index];
    const auto& note = queriedNote.note;
    std::cout << "  queriedNote[" << index << "]:"
              << " pitch=" << note.pitch
              << " start=" << note.startSeconds
              << " duration=" << note.durationSeconds
              << " channel=" << note.channel
              << " track=" << note.track
              << " startsBeforeRange=" << (queriedNote.startsBeforeRange ? "true" : "false")
              << " endsAfterRange=" << (queriedNote.endsAfterRange ? "true" : "false")
              << '\n';
  }
}

} // namespace

Application::~Application() {
  shutdown();
}

bool Application::initialize() {
  loadStartupMidiIfPresent();

  const WindowConfig windowConfig{
    .title = "KeyWave",
    .width = 1280,
    .height = 720,
  };

  if (!m_window.initialize(windowConfig)) {
    std::cerr << "Application initialization failed: window could not be created.\n";
    return false;
  }

  m_renderer = std::make_unique<OpenGLRendererBackend>(
    Window::nativeProcAddressLoader(),
    Color{.r=0.025f, .g=0.03f, .b=0.04f, .a=1.0f}
  );

  if (!m_renderer->initialize()) {
    std::cerr << "Application initialization failed: renderer could not be initialized.\n";
    m_renderer.reset();
    m_window.shutdown();
    return false;
  }

  m_initialized = true;
  return true;
}

void Application::run() const {
  if (!m_initialized) {
    return;
  }

  while (!m_window.shouldClose()) {
    Window::pollEvents();
    m_renderer->beginFrame();
    m_renderer->endFrame();
    m_window.swapBuffers();
  }
}

void Application::shutdown() {
  if (m_renderer) {
    m_renderer->shutdown();
    m_renderer.reset();
  }

  m_window.shutdown();
  m_initialized = false;
}
