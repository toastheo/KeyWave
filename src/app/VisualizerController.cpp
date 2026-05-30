#include "app/VisualizerController.hpp"

#include <utility>

#include "app/PlaybackTransportControls.hpp"
#include "app/VisualizationSettingsAdapters.hpp"
#include "app/VisualizationSettingsPanelControls.hpp"
#include "fallingnotes/PianoRollSceneBuilder.hpp"

VisualizerController::VisualizerController()
    : VisualizerController(AppSettings{}, nullDiagnosticSink())
{}

VisualizerController::VisualizerController(DiagnosticSink& diagnostics)
    : VisualizerController(AppSettings{}, diagnostics)
{}

VisualizerController::VisualizerController(AppSettings settings)
    : VisualizerController(std::move(settings), nullDiagnosticSink())
{}

VisualizerController::VisualizerController(AppSettings settings, DiagnosticSink& diagnostics)
    : m_diagnostics(&diagnostics)
    , m_playbackTransport(diagnostics)
{
  setSettings(std::move(settings));
}

void VisualizerController::setSettings(AppSettings settings)
{
  m_settings = sanitizeAppSettings(std::move(settings));
}

AppSettings& VisualizerController::settings()
{
  return m_settings;
}

const AppSettings& VisualizerController::settings() const
{
  return m_settings;
}

void VisualizerController::setTimeline(std::optional<MidiTimeline> timeline)
{
  m_timeline = std::move(timeline);
}

bool VisualizerController::hasTimeline() const
{
  return m_timeline.has_value();
}

double VisualizerController::durationSeconds() const
{
  return m_timeline.has_value() ? m_timeline->lengthSeconds() : 0.0;
}

PlaybackTransport& VisualizerController::playbackTransport()
{
  return m_playbackTransport;
}

const PlaybackTransport& VisualizerController::playbackTransport() const
{
  return m_playbackTransport;
}

bool VisualizerController::visualizationSettingsPanelVisible() const
{
  return m_visualizationSettingsPanelVisible;
}

void VisualizerController::setVisualizationSettingsPanelVisible(bool visible)
{
  m_visualizationSettingsPanelVisible = visible;
}

void VisualizerController::handleInput(const std::span<const Key> pressedKeys,
                                       const bool imguiWantsKeyboardCapture)
{
  for (const auto key : pressedKeys) {
    applyVisualizationSettingsPanelControl(key, m_visualizationSettingsPanelVisible);

    if (!imguiWantsKeyboardCapture) {
      applyPlaybackTransportControl(
        key, m_playbackTransport, *m_diagnostics, m_settings.playbackControls);
    }
  }
}

void VisualizerController::update(const double elapsedSeconds)
{
  m_playbackTransport.update(elapsedSeconds);
}

RenderScene VisualizerController::buildScene() const
{
  if (!m_timeline.has_value()) {
    return {};
  }

  return PianoRollSceneBuilder::build(
    *m_timeline,
    m_playbackTransport.currentTimeSeconds(),
    pianoRollSceneConfigFromSettings(m_settings.fallingNotes, m_settings.keyboard),
    *m_diagnostics);
}
