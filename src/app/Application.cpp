#include "app/Application.hpp"

#include <algorithm>
#include <chrono>
#include <memory>
#include <sstream>
#include <string_view>
#include <thread>
#include <utility>

#include "app/StartupDataLoader.hpp"
#include "midi/MidiFileLoader.hpp"
#include "platform/MidiFileDialog.hpp"
#include "render/RenderScene.hpp"
#include "render_opengl/OpenGLRendererBackend.hpp"
#include "ui/TransportControls.hpp"
#include "ui/VisualizationSettingsPanel.hpp"

namespace {

PlatformWindowDisplayMode platformDisplayMode(const WindowDisplayMode displayMode)
{
  switch (displayMode) {
    case WindowDisplayMode::Windowed:
      return PlatformWindowDisplayMode::Windowed;
    case WindowDisplayMode::BorderlessFullscreen:
      return PlatformWindowDisplayMode::BorderlessFullscreen;
    case WindowDisplayMode::ExclusiveFullscreen:
      return PlatformWindowDisplayMode::ExclusiveFullscreen;
  }
  return PlatformWindowDisplayMode::Windowed;
}

} // namespace

Application::Application(AppConfig config)
    : Application(std::move(config), consoleDiagnosticSink())
{}

Application::Application(AppConfig config, DiagnosticSink& diagnostics)
    : m_config(std::move(config))
    , m_diagnostics(diagnostics)
    , m_visualizerController(diagnostics)
{}

Application::~Application()
{
  shutdown();
}

bool Application::initialize()
{
  reportInfo(m_diagnostics, "Settings path: " + m_settingsStorage.path().string());
  if (auto loadedSettings = m_settingsStorage.load(m_diagnostics); loadedSettings.has_value()) {
    m_visualizerController.setSettings(*loadedSettings);
    reportInfo(m_diagnostics, "Settings loaded.");
  }
  reportInfo(m_diagnostics, "Settings will be saved on exit.");

  auto startupData = StartupDataLoader::load(m_config, m_midiLibraryStore, m_diagnostics);
  m_visualizerController.setTimeline(std::move(startupData.timeline));
  m_activeImportedMidiId = std::move(startupData.importedMidiId);
  refreshImportedMidiFiles();
  const auto& settings = m_visualizerController.settings();

  const WindowConfig windowConfig{
    .title = settings.window.title,
    .width = settings.window.width,
    .height = settings.window.height,
    .vsyncEnabled = settings.window.vsyncEnabled,
  };

  if (!m_window.initialize(windowConfig, m_diagnostics)) {
    reportError(m_diagnostics, "Application initialization failed: window could not be created.");
    return false;
  }

  m_renderer = std::make_unique<OpenGLRendererBackend>(Window::nativeProcAddressLoader(),
                                                       settings.renderer.clearColor,
                                                       m_diagnostics);

  if (!m_renderer->initialize()) {
    reportError(m_diagnostics,
                "Application initialization failed: renderer could not be initialized.");
    m_renderer.reset();
    m_window.shutdown();
    return false;
  }
  m_renderer->setFramebufferSize(m_window.framebufferSize());

  if (!m_imguiLayer.initialize(m_window.nativeHandle(), m_diagnostics)) {
    reportError(m_diagnostics,
                "Application initialization failed: UI layer could not be initialized.");
    m_renderer->shutdown();
    m_renderer.reset();
    m_window.shutdown();
    return false;
  }

  m_appliedWindowSettings = settings.window;
  m_appliedWindowSettings.displayMode = WindowDisplayMode::Windowed;
  applyWindowSettings();

  if (m_visualizerController.hasTimeline()) {
    m_visualizerController.playbackTransport().play();
    reportInfo(m_diagnostics, "Playback started.");
  }

  m_initialized = true;
  return true;
}

void Application::applyWindowSettings()
{
  auto& settings = m_visualizerController.settings();
  const auto sanitizedWindow = sanitizeWindowSettings(settings.window);
  settings.window = sanitizedWindow;

  if (sanitizedWindow.displayMode != m_appliedWindowSettings.displayMode ||
      sanitizedWindow.width != m_appliedWindowSettings.width ||
      sanitizedWindow.height != m_appliedWindowSettings.height) {
    const auto requestedMode = sanitizedWindow.displayMode;
    const bool applied = m_window.setDisplayMode(platformDisplayMode(requestedMode),
                                                 WindowedSize{.width = sanitizedWindow.width,
                                                              .height = sanitizedWindow.height},
                                                 m_diagnostics);
    if (!applied && requestedMode != WindowDisplayMode::Windowed) {
      settings.window.displayMode = WindowDisplayMode::Windowed;
    }
  }

  if (sanitizedWindow.displayMode == WindowDisplayMode::Windowed &&
      (sanitizedWindow.width != m_appliedWindowSettings.width ||
       sanitizedWindow.height != m_appliedWindowSettings.height)) {
    m_window.setWindowedSize(
      WindowedSize{.width = sanitizedWindow.width, .height = sanitizedWindow.height});
  }

  if (sanitizedWindow.vsyncEnabled != m_appliedWindowSettings.vsyncEnabled) {
    m_window.setVsyncEnabled(sanitizedWindow.vsyncEnabled);
  }

  m_appliedWindowSettings = settings.window;
}

void Application::refreshImportedMidiFiles()
{
  m_importedMidiFiles = m_midiLibraryStore.listImportedFiles(m_diagnostics);
  std::ranges::sort(m_importedMidiFiles,
                    [](const ImportedMidiFile& left, const ImportedMidiFile& right) {
                      if (left.lastOpenedAt != right.lastOpenedAt) {
                        return left.lastOpenedAt > right.lastOpenedAt;
                      }
                      if (left.importedAt != right.importedAt) {
                        return left.importedAt > right.importedAt;
                      }
                      return left.displayName < right.displayName;
                    });
}

bool Application::loadImportedMidiFile(const std::string_view id)
{
  const auto storedMidiPath = m_midiLibraryStore.importedFilePath(id, m_diagnostics);
  if (!storedMidiPath.has_value()) {
    reportWarning(m_diagnostics,
                  "Warning: imported MIDI file is unavailable. Keeping the current MIDI file.");
    return false;
  }

  reportInfo(m_diagnostics, "Loading MIDI file: " + storedMidiPath->string());
  auto timeline = MidiFileLoader::loadFromFile(*storedMidiPath, m_diagnostics);
  if (!timeline.has_value()) {
    reportWarning(m_diagnostics, "Warning: MIDI loading failed. Keeping the current MIDI file.");
    return false;
  }

  if (!m_midiLibraryStore.setLastActiveMidiId(id, m_diagnostics)) {
    reportWarning(m_diagnostics, "Warning: could not mark imported MIDI file as last active");
  }

  std::ostringstream message;
  message << "Loaded MIDI file with " << timeline->notes().size()
          << " note(s), length=" << timeline->lengthSeconds() << "s.";
  reportInfo(m_diagnostics, message.str());

  m_activeImportedMidiId = std::string(id);
  refreshImportedMidiFiles();
  m_visualizerController.replaceTimelineAndPlayFromStart(std::move(timeline));
  return true;
}

void Application::renameImportedMidiFile(const std::string_view id, const std::string_view displayName)
{
  if (!m_midiLibraryStore.renameImportedMidiFile(id, displayName, m_diagnostics)) {
    reportWarning(m_diagnostics, "Warning: could not rename imported MIDI file.");
    return;
  }

  refreshImportedMidiFiles();
}

void Application::handleVisualizationSettingsPanelAction(
  const VisualizationSettingsPanelResult& result)
{
  if (result.action == VisualizationSettingsPanelAction::RenameImportedMidiFile) {
    renameImportedMidiFile(result.selectedImportedMidiId, result.renamedImportedMidiDisplayName);
    return;
  }

  if (result.action == VisualizationSettingsPanelAction::LoadImportedMidiFile) {
    loadImportedMidiFile(result.selectedImportedMidiId);
    return;
  }

  if (result.action != VisualizationSettingsPanelAction::LoadMidiFile) {
    return;
  }

  m_visualizerController.suppressNextPlaybackUpdate();

  const auto midiPath = MidiFileDialog::open(m_diagnostics);
  if (!midiPath.has_value()) {
    return;
  }

  auto importedMidi = m_midiLibraryStore.importFile(*midiPath, m_diagnostics);
  if (!importedMidi.has_value()) {
    reportWarning(m_diagnostics, "Warning: MIDI import failed. Keeping the current MIDI file.");
    return;
  }

  if (importedMidi->alreadyImported) {
    reportInfo(m_diagnostics,
               "Selected already imported MIDI file: " + importedMidi->file.displayName);
  }
  else {
    reportInfo(m_diagnostics, "Imported MIDI file: " + importedMidi->file.displayName);
  }

  loadImportedMidiFile(importedMidi->file.id);
}

void Application::paceFrame(const std::chrono::steady_clock::time_point frameStart)
{
  const auto& windowSettings = m_visualizerController.settings().window;
  if (windowSettings.vsyncEnabled || windowSettings.fpsLimit == unlimitedFpsLimit) {
    return;
  }

  const auto targetFrameDuration =
    std::chrono::duration<double>(1.0 / static_cast<double>(windowSettings.fpsLimit));
  const auto elapsed = std::chrono::steady_clock::now() - frameStart;
  if (elapsed < targetFrameDuration) {
    std::this_thread::sleep_for(targetFrameDuration - elapsed);
  }
}

void Application::run()
{
  if (!m_initialized) {
    return;
  }

  auto previousFrameTime = std::chrono::steady_clock::now();

  while (!m_window.shouldClose()) {
    const auto frameStartTime = std::chrono::steady_clock::now();
    const std::chrono::duration<double> elapsed = frameStartTime - previousFrameTime;
    previousFrameTime = frameStartTime;

    Window::pollEvents();
    const auto pressedKeys = m_window.consumePressedKeys();
    const auto imguiWantsKeyboardCapture = m_imguiLayer.wantsKeyboardCapture();
    m_visualizerController.handleInput(pressedKeys, imguiWantsKeyboardCapture);

    m_renderer->setFramebufferSize(m_window.framebufferSize());
    m_visualizerController.update(elapsed.count());

    m_imguiLayer.beginFrame();
    TransportControls::render(m_visualizerController.playbackTransport(),
                              m_visualizerController.durationSeconds(),
                              m_visualizerController.settings().playbackControls);
    if (m_visualizerController.visualizationSettingsPanelVisible()) {
      const auto settingsAction =
        VisualizationSettingsPanel::render(m_visualizerController.settings(),
                                           m_visualizerController.playbackTransport(),
                                           m_importedMidiFiles,
                                           m_activeImportedMidiId.value_or(""));
      handleVisualizationSettingsPanelAction(settingsAction);
    }
    applyWindowSettings();
    m_renderer->setClearColor(m_visualizerController.settings().renderer.clearColor);

    const auto scene = m_visualizerController.buildScene();

    m_renderer->setView(scene.view);
    m_renderer->beginFrame();
    m_renderer->submit(scene.commands);
    m_renderer->endFrame();
    m_imguiLayer.endFrame();
    m_window.swapBuffers();
    paceFrame(frameStartTime);
  }
}

void Application::shutdown()
{
  if (!m_settingsSaved) {
    if (m_settingsStorage.save(m_visualizerController.settings(), m_diagnostics)) {
      reportInfo(m_diagnostics, "Settings saved: " + m_settingsStorage.path().string());
    }
    m_settingsSaved = true;
  }

  m_imguiLayer.shutdown();

  if (m_renderer) {
    m_renderer->shutdown();
    m_renderer.reset();
  }

  m_window.shutdown();
  m_visualizerController.setTimeline(std::nullopt);
  m_visualizerController.playbackTransport().stop();
  m_initialized = false;
}
