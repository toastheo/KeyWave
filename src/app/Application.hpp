#pragma once

#include <chrono>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "app/AppConfig.hpp"
#include "app/MidiLibraryStore.hpp"
#include "app/SettingsStorage.hpp"
#include "app/VisualizerController.hpp"
#include "diagnostics/Diagnostics.hpp"
#include "platform/Window.hpp"
#include "render/RendererBackend.hpp"
#include "ui/ImGuiLayer.hpp"

enum class VisualizationSettingsPanelAction : std::uint8_t;
class PianoSynth;
struct VisualizationSettingsPanelResult;

class Application
{
public:
  explicit Application(AppConfig config = {});
  ~Application();

  Application(const Application&) = delete;
  Application& operator=(const Application&) = delete;

  bool initialize();
  void run();
  void shutdown();

private:
  Application(AppConfig config, DiagnosticSink& diagnostics);

  void applyWindowSettings();
  void refreshImportedMidiFiles();
  bool loadImportedMidiFile(std::string_view id);
  void renameImportedMidiFile(std::string_view id, std::string_view displayName);
  void removeImportedMidiFile(std::string_view id);
  void handleVisualizationSettingsPanelAction(const VisualizationSettingsPanelResult& result);
  void paceFrame(std::chrono::steady_clock::time_point frameStart);

  AppConfig m_config;
  DiagnosticSink& m_diagnostics;
  SettingsStorage m_settingsStorage;
  MidiLibraryStore m_midiLibraryStore;
  std::vector<ImportedMidiFile> m_importedMidiFiles;
  std::optional<std::string> m_activeImportedMidiId;
  std::unique_ptr<PianoSynth> m_pianoSynth;
  VisualizerController m_visualizerController;
  Window m_window;
  std::unique_ptr<RendererBackend> m_renderer;
  ImGuiLayer m_imguiLayer;
  WindowSettings m_appliedWindowSettings;
  bool m_initialized = false;
  bool m_settingsSaved = false;
};
