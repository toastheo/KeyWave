#pragma once

#include <iosfwd>
#include <optional>
#include <span>

#include "app/AppSettings.hpp"
#include "input/Key.hpp"
#include "midi/MidiTimeline.hpp"
#include "playback/PlaybackTransport.hpp"
#include "render/RenderScene.hpp"

class VisualizerController
{
public:
  VisualizerController();
  explicit VisualizerController(AppSettings settings);

  void setSettings(AppSettings settings);
  [[nodiscard]] AppSettings& settings();
  [[nodiscard]] const AppSettings& settings() const;

  void setTimeline(std::optional<MidiTimeline> timeline);
  [[nodiscard]] bool hasTimeline() const;
  [[nodiscard]] double durationSeconds() const;

  [[nodiscard]] PlaybackTransport& playbackTransport();
  [[nodiscard]] const PlaybackTransport& playbackTransport() const;

  [[nodiscard]] bool visualizationSettingsPanelVisible() const;
  void setVisualizationSettingsPanelVisible(bool visible);

  void handleInput(std::span<const Key> pressedKeys,
                   bool imguiWantsKeyboardCapture,
                   std::ostream& output);
  void update(double elapsedSeconds);
  [[nodiscard]] RenderScene buildScene() const;

private:
  AppSettings m_settings;
  std::optional<MidiTimeline> m_timeline;
  PlaybackTransport m_playbackTransport;
  bool m_visualizationSettingsPanelVisible = true;
};
