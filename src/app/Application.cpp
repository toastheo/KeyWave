#include "app/Application.hpp"

#include <algorithm>
#include <filesystem>
#include <iostream>
#include <memory>
#include <utility>
#include <variant>

#include "fallingnotes/FallingNotesLayout.hpp"
#include "fallingnotes/FallingNotesRenderAdapter.hpp"
#include "midi/MidiFileLoader.hpp"
#include "midi/MidiTimelineQuery.hpp"
#include "render/RenderCommand.hpp"
#include "render/RenderTypes.hpp"
#include "render/RendererView.hpp"
#include "render_opengl/OpenGLRendererBackend.hpp"

namespace {

struct StartupRenderScene
{
  std::vector<RenderCommand> commands;
  RendererView view;
};

void printResolvedPathIfRelative(const std::filesystem::path& path)
{
  if (!path.is_relative()) {
    return;
  }

  std::error_code errorCode;
  const auto absolutePath = std::filesystem::absolute(path, errorCode);
  if (errorCode) {
    std::cout << "  resolved absolute path unavailable: " << errorCode.message() << '\n';
    return;
  }

  std::cout << "  resolved absolute path: " << absolutePath.string() << '\n';
}

RendererView rendererViewForLayout(const FallingNotesLayoutResult& layoutResult)
{
  const RendererView view{
    .visibleWorldRect =
      WorldRect{
        .x = 0.0,
        .y = 0.0,
        .width = layoutResult.contentWidth,
        .height = layoutResult.contentHeight,
      },
  };

  if (!isValid(view.visibleWorldRect)) {
    std::cerr << "Using default renderer view because the falling-notes layout size is invalid"
              << " (contentWidth=" << layoutResult.contentWidth
              << ", contentHeight=" << layoutResult.contentHeight << ").\n";
    return RendererView{};
  }

  return view;
}

StartupRenderScene loadStartupMidiIfPresent(const AppConfig& config)
{
  if (!config.midiFilePath.has_value()) {
    std::cout << "No MIDI file provided. Starting with an empty window.\n";
    return {};
  }

  const auto& midiPath = *config.midiFilePath;
  std::cout << "Loading MIDI file: " << midiPath.string() << '\n';
  printResolvedPathIfRelative(midiPath);

  std::error_code errorCode;
  if (!std::filesystem::exists(midiPath, errorCode)) {
    std::cerr << "Warning: MIDI file does not exist: " << midiPath.string();
    if (errorCode) {
      std::cerr << " (" << errorCode.message() << ')';
    }
    std::cerr << ". Opening an empty window.\n";
    return {};
  }

  if (!std::filesystem::is_regular_file(midiPath, errorCode)) {
    std::cerr << "Warning: MIDI path is not a regular file: " << midiPath.string();
    if (errorCode) {
      std::cerr << " (" << errorCode.message() << ')';
    }
    std::cerr << ". Opening an empty window.\n";
    return {};
  }

  const auto timeline = MidiFileLoader::loadFromFile(midiPath);
  if (!timeline.has_value()) {
    std::cerr << "Warning: MIDI loading failed. Opening an empty window.\n";
    return {};
  }

  const MidiTimelineQuery query(*timeline);
  constexpr auto viewport = FallingNotesViewport{
    .pitchRange = PitchRange{.minPitch = 21, .maxPitch = 108},
    .currentTimeSeconds = 0.0,
    .lookAheadSeconds = 10.0,
    .visiblePastSeconds = 0.0,
  };
  const auto notes = query.findNotes(TimelineViewport{
    .timeRange =
      TimeRange{
        .startSeconds = viewport.currentTimeSeconds - viewport.visiblePastSeconds,
        .endSeconds = viewport.currentTimeSeconds + viewport.lookAheadSeconds,
      },
    .pitchRange = viewport.pitchRange,
  });

  const auto layoutResult = FallingNotesLayout::build(notes, viewport);
  const auto renderCommands = FallingNotesRenderAdapter::buildCommands(layoutResult);
  const auto rendererView = rendererViewForLayout(layoutResult);

  std::cout << "MIDI startup falling-notes layout\n";
  std::cout << "  queried notes: " << notes.size() << '\n';
  std::cout << "  layout notes: " << layoutResult.notes.size() << '\n';
  std::cout << "  render commands: " << renderCommands.size() << '\n';
  std::cout << "  pitch lane count: " << layoutResult.pitchLaneCount << '\n';
  std::cout << "  contentWidth: " << layoutResult.contentWidth << '\n';
  std::cout << "  contentHeight: " << layoutResult.contentHeight << '\n';
  std::cout << "  currentTimeSeconds: " << layoutResult.currentTimeSeconds << '\n';
  std::cout << "  lookAheadSeconds: " << layoutResult.lookAheadSeconds << '\n';
  std::cout << "  visiblePastSeconds: " << layoutResult.visiblePastSeconds << '\n';

  const auto previewCount = std::min<std::size_t>(renderCommands.size(), 5);
  for (std::size_t index = 0; index < previewCount; ++index) {
    if (!std::holds_alternative<DrawRectCommand>(renderCommands[index])) {
      continue;
    }

    const auto& [rect, color] = std::get<DrawRectCommand>(renderCommands[index]);
    std::cout << "  drawRect[" << index << "]:"
              << " x=" << rect.x << " y=" << rect.y << " width=" << rect.width
              << " height=" << rect.height << " color=(" << color.r << ", " << color.g << ", "
              << color.b << ", " << color.a << ")\n";
  }

  return StartupRenderScene{
    .commands = renderCommands,
    .view = rendererView,
  };
}

} // namespace

Application::Application(AppConfig config)
  : m_config(std::move(config))
{
}

Application::~Application()
{
  shutdown();
}

bool Application::initialize()
{
  auto startupScene = loadStartupMidiIfPresent(m_config);
  m_startupRenderCommands = std::move(startupScene.commands);
  m_startupRendererView = startupScene.view;

  const WindowConfig windowConfig{
    .title = "KeyWave",
    .width = 1280,
    .height = 720,
  };

  if (!m_window.initialize(windowConfig)) {
    std::cerr << "Application initialization failed: window could not be created.\n";
    return false;
  }

  m_renderer =
    std::make_unique<OpenGLRendererBackend>(Window::nativeProcAddressLoader(),
                                            Color{.r = 0.025f, .g = 0.03f, .b = 0.04f, .a = 1.0f});

  if (!m_renderer->initialize()) {
    std::cerr << "Application initialization failed: renderer could not be initialized.\n";
    m_renderer.reset();
    m_window.shutdown();
    return false;
  }

  m_renderer->setView(m_startupRendererView);

  m_initialized = true;
  return true;
}

void Application::run() const
{
  if (!m_initialized) {
    return;
  }

  while (!m_window.shouldClose()) {
    Window::pollEvents();
    m_renderer->beginFrame();
    m_renderer->submit(m_startupRenderCommands);
    m_renderer->endFrame();
    m_window.swapBuffers();
  }
}

void Application::shutdown()
{
  if (m_renderer) {
    m_renderer->shutdown();
    m_renderer.reset();
  }

  m_window.shutdown();
  m_startupRenderCommands.clear();
  m_initialized = false;
}
