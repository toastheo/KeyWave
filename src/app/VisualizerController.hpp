#pragma once

#include <optional>
#include <span>

#include "app/AppSettings.hpp"
#include "audio/NullPianoSynth.hpp"
#include "audio/PianoSynth.hpp"
#include "audio/TimelineAudioScheduler.hpp"
#include "diagnostics/Diagnostics.hpp"
#include "input/Key.hpp"
#include "midi/MidiTimeline.hpp"
#include "playback/PlaybackTransport.hpp"
#include "render/RenderScene.hpp"

class VisualizerController
{
public:
  VisualizerController();
  explicit VisualizerController(DiagnosticSink& diagnostics);

  void setSettings(AppSettings settings);
  [[nodiscard]] const AppSettings& settings() const;

  void setTimeline(std::optional<MidiTimeline> timeline);
  void replaceTimelineAndPlayFromStart(std::optional<MidiTimeline> timeline);
  [[nodiscard]] bool hasTimeline() const;
  [[nodiscard]] double durationSeconds() const;
  [[nodiscard]] double sourceBpmAtPlaybackPosition() const;

  [[nodiscard]] PlaybackTransport& playbackTransport();
  [[nodiscard]] const PlaybackTransport& playbackTransport() const;

  [[nodiscard]] bool visualizationSettingsPanelVisible() const;

  void handleInput(std::span<const Key> pressedKeys, bool imguiWantsKeyboardCapture);
  void update(double elapsedSeconds);
  void suppressNextPlaybackUpdate();
  [[nodiscard]] RenderScene buildScene() const;

private:
  DiagnosticSink& m_diagnostics;
  AppSettings m_settings;
  std::optional<MidiTimeline> m_timeline;
  PlaybackTransport m_playbackTransport;
  NullPianoSynth m_nullPianoSynth;
  TimelineAudioScheduler m_audioScheduler;
  bool m_visualizationSettingsPanelVisible = true;
  bool m_skipNextPlaybackUpdate = false;
};
