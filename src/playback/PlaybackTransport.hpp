#pragma once

#include <cstdint>

#include "diagnostics/Diagnostics.hpp"

enum class PlaybackState : std::uint8_t
{
  Stopped,
  Playing,
  Paused,
};

class PlaybackTransport
{
public:
  explicit PlaybackTransport(DiagnosticSink& diagnostics = nullDiagnosticSink());

  void play();
  void pause();
  void stop();
  void seek(double seconds);
  void update(double deltaSeconds);

  [[nodiscard]] double currentTimeSeconds() const;
  [[nodiscard]] PlaybackState state() const;

  void setPlaybackRate(double rate);
  [[nodiscard]] double playbackRate() const;

  void setEffectiveBpm(double sourceBpm, double targetBpm);
  [[nodiscard]] double effectiveBpm(double sourceBpm) const;

private:
  DiagnosticSink& m_diagnostics;
  double m_currentTimeSeconds = 0.0;
  double m_playbackRate = 1.0;
  PlaybackState m_state = PlaybackState::Stopped;
};
