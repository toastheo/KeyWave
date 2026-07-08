#include "app/VisualizerController.hpp"

#include <optional>
#include <span>
#include <utility>

#include "app/AppSettings.hpp"
#include "app/PlaybackTransportControls.hpp"
#include "app/VisualizationSettingsAdapters.hpp"
#include "app/VisualizationSettingsPanelControls.hpp"
#include "audio/PianoSynth.hpp"
#include "audio/TimelineAudioScheduler.hpp"
#include "diagnostics/Diagnostics.hpp"
#include "fallingnotes/PianoRollSceneBuilder.hpp"
#include "input/Key.hpp"
#include "midi/MidiTimeline.hpp"
#include "midi/MidiTypes.hpp"
#include "playback/PlaybackTransport.hpp"
#include "render/RenderScene.hpp"

VisualizerController::VisualizerController()
    : VisualizerController(nullDiagnosticSink())
{}

VisualizerController::VisualizerController(DiagnosticSink& diagnostics)
    : m_diagnostics(diagnostics)
    , m_playbackTransport(diagnostics)
    , m_audioScheduler(m_nullPianoSynth)
{
  setSettings(AppSettings{});
}

VisualizerController::VisualizerController(PianoSynth& pianoSynth)
    : VisualizerController(AppSettings{}, nullDiagnosticSink(), pianoSynth)
{}

VisualizerController::VisualizerController(AppSettings settings,
                                           DiagnosticSink& diagnostics,
                                           PianoSynth& pianoSynth)
    : m_diagnostics(diagnostics)
    , m_playbackTransport(diagnostics)
    , m_audioScheduler(pianoSynth)
{
  setSettings(std::move(settings));
}

void VisualizerController::setSettings(AppSettings settings)
{
  m_settings = sanitizeAppSettings(std::move(settings));
}

const AppSettings& VisualizerController::settings() const
{
  return m_settings;
}

void VisualizerController::setTimeline(std::optional<MidiTimeline> timeline)
{
  m_timeline = std::move(timeline);

  if (m_timeline.has_value()) {
    m_audioScheduler.setTimeline(*m_timeline);
    return;
  }

  m_audioScheduler.setTimeline(MidiTimeline{});
}

void VisualizerController::replaceTimelineAndPlayFromStart(std::optional<MidiTimeline> timeline)
{
  setTimeline(std::move(timeline));
  m_playbackTransport.seek(0.0);
  if (m_timeline.has_value()) {
    m_playbackTransport.play();

    // Loading a file blocks the frame so we skip one tick so the replacement starts exactly at zero.
    suppressNextPlaybackUpdate();
  }
}

bool VisualizerController::hasTimeline() const
{
  return m_timeline.has_value();
}

double VisualizerController::durationSeconds() const
{
  return m_timeline.has_value() ? m_timeline->lengthSeconds() : 0.0;
}

double VisualizerController::sourceBpmAtPlaybackPosition() const
{
  if (!m_timeline.has_value()) {
    return defaultMidiBpm;
  }

  return m_timeline->sourceBpmAt(m_playbackTransport.currentTimeSeconds());
}

PlaybackTransport& VisualizerController::playbackTransport()
{
  return m_playbackTransport;
}

const PlaybackTransport& VisualizerController::playbackTransport() const
{
  return m_playbackTransport;
}

TimelineAudioScheduler& VisualizerController::audioScheduler()
{
  return m_audioScheduler;
}

bool VisualizerController::visualizationSettingsPanelVisible() const
{
  return m_visualizationSettingsPanelVisible;
}

void VisualizerController::handleInput(const std::span<const Key> pressedKeys,
                                       const bool imguiWantsKeyboardCapture)
{
  // We want to keep the settings panel toggle global so that the settings panel can be hidden
  // even when it's in focus.
  for (const auto key : pressedKeys) {
    applyVisualizationSettingsPanelControl(key, m_visualizationSettingsPanelVisible);

    if (!imguiWantsKeyboardCapture) {
      applyPlaybackTransportControl(key,
                                    m_playbackTransport,
                                    m_diagnostics,
                                    m_audioScheduler,
                                    m_settings.playbackControls,
                                    sourceBpmAtPlaybackPosition());
    }
  }
}

void VisualizerController::update(const double elapsedSeconds)
{
  if (m_skipNextPlaybackUpdate) {
    m_skipNextPlaybackUpdate = false;
    return;
  }

  const auto previousTimeSeconds = m_playbackTransport.currentTimeSeconds();
  m_playbackTransport.update(elapsedSeconds);
  if (m_playbackTransport.state() == PlaybackState::Playing) {
    m_audioScheduler.update(previousTimeSeconds, m_playbackTransport.currentTimeSeconds());
  }
}

void VisualizerController::suppressNextPlaybackUpdate()
{
  m_skipNextPlaybackUpdate = true;
}

RenderScene VisualizerController::buildScene() const
{
  if (!m_timeline.has_value()) {
    return {};
  }

  return PianoRollSceneBuilder::build(*m_timeline,
                                      m_playbackTransport.currentTimeSeconds(),
                                      pianoRollSceneConfigFromSettings(m_settings.fallingNotes,
                                                                       m_settings.keyboard),
                                      m_diagnostics);
}
