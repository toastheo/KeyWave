add_library(keywave_core INTERFACE
        core/CoreTypes.hpp
)

target_include_directories(keywave_core
        INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

add_library(keywave_midi STATIC
        midi/MidiTypes.hpp
        midi/MidiTrack.hpp
        midi/MidiTrack.cpp
        midi/MidiTimeline.hpp
        midi/MidiTimeline.cpp
        midi/MidiTimelineQuery.hpp
        midi/MidiTimelineQuery.cpp
        midi/MidiFileLoader.hpp
        midi/MidiFileLoader.cpp
)

target_include_directories(keywave_midi
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(keywave_midi
        PUBLIC
        keywave_core
        PRIVATE
        midifile
)

add_library(keywave_render STATIC
        render/RendererBackend.hpp
        render/RenderCommand.hpp
        render/RenderScene.hpp
        render/RenderTypes.hpp
        render/RendererView.hpp
        render/RendererView.cpp
)

target_include_directories(keywave_render
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(keywave_render
        PUBLIC
        keywave_core
)

add_library(keywave_app_settings STATIC
        app/AppSettings.hpp
        app/AppSettings.cpp
        app/AppSettingsSerializer.hpp
        app/AppSettingsSerializer.cpp
        app/SettingsStorage.hpp
        app/SettingsStorage.cpp
)

target_include_directories(keywave_app_settings
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${PROJECT_SOURCE_DIR}/external
)

target_link_libraries(keywave_app_settings
        PUBLIC
        keywave_core
)

add_library(keywave_keyboard STATIC
        keyboard/KeyboardTypes.hpp
        keyboard/KeyboardState.hpp
        keyboard/KeyboardState.cpp
        keyboard/KeyboardGeometry.hpp
        keyboard/KeyboardGeometry.cpp
        keyboard/KeyboardLayout.hpp
        keyboard/KeyboardLayout.cpp
        keyboard/KeyboardRenderAdapter.hpp
        keyboard/KeyboardRenderAdapter.cpp
)

target_include_directories(keywave_keyboard
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(keywave_keyboard
        PUBLIC
        keywave_core
        keywave_midi
        keywave_render
)

add_library(keywave_fallingnotes STATIC
        fallingnotes/FallingNotesTypes.hpp
        fallingnotes/FallingNotesLayout.hpp
        fallingnotes/FallingNotesLayout.cpp
        fallingnotes/FallingNotesRenderAdapter.hpp
        fallingnotes/FallingNotesRenderAdapter.cpp
        fallingnotes/FallingNotesSceneBuilder.hpp
        fallingnotes/FallingNotesSceneBuilder.cpp
)

target_include_directories(keywave_fallingnotes
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(keywave_fallingnotes
        PUBLIC
        keywave_keyboard
        keywave_midi
        keywave_render
)

add_library(keywave_playback STATIC
        playback/PlaybackTransport.hpp
        playback/PlaybackTransport.cpp
)

target_include_directories(keywave_playback
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

add_library(keywave_platform STATIC
        input/Key.hpp
        platform/Window.hpp
        platform/Window.cpp
)

target_include_directories(keywave_platform
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(keywave_platform
        PUBLIC
        keywave_render
        PRIVATE
        glfw
)

add_library(keywave_app_core STATIC
        app/AppConfig.hpp
        app/AppConfig.cpp
        app/PlaybackTransportAction.hpp
        app/PlaybackTransportAction.cpp
        app/PlaybackTransportControls.hpp
        app/PlaybackTransportControls.cpp
        app/VisualizationSettingsPanelControls.hpp
        app/VisualizationSettingsPanelControls.cpp
        app/VisualizationSettingsAdapters.hpp
        app/VisualizationSettingsAdapters.cpp
        app/StartupDataLoader.hpp
        app/StartupDataLoader.cpp
        app/VisualizerController.hpp
        app/VisualizerController.cpp
)

target_include_directories(keywave_app_core
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(keywave_app_core
        PUBLIC
        keywave_app_settings
        keywave_fallingnotes
        keywave_keyboard
        keywave_midi
        keywave_playback
)

add_library(keywave_ui STATIC
        ui/ImGuiLayer.hpp
        ui/ImGuiLayer.cpp
        ui/TransportControls.hpp
        ui/TransportControls.cpp
        ui/TransportControlsConfig.hpp
        ui/TransportControlsConfig.cpp
        ui/TransportTime.hpp
        ui/TransportTime.cpp
        ui/VisualizationSettingsPanel.hpp
        ui/VisualizationSettingsPanel.cpp
)

target_include_directories(keywave_ui
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(keywave_ui
        PUBLIC
        keywave_app_core
        keywave_playback
        imgui
        PRIVATE
        glfw
        OpenGL::GL
)

add_library(keywave_render_opengl STATIC
        render_opengl/OpenGLRendererBackend.hpp
        render_opengl/OpenGLRendererBackend.cpp
        render_opengl/OpenGLShader.hpp
        render_opengl/OpenGLShader.cpp
)

target_include_directories(keywave_render_opengl
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(keywave_render_opengl
        PUBLIC
        keywave_platform
        keywave_render
        PRIVATE
        glad
        OpenGL::GL
)

add_library(keywave_app STATIC
        app/Application.hpp
        app/Application.cpp
)

target_include_directories(keywave_app
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(keywave_app
        PUBLIC
        keywave_app_core
        keywave_fallingnotes
        keywave_midi
        keywave_platform
        keywave_playback
        keywave_render
        keywave_render_opengl
        keywave_ui
)

add_executable(KeyWave
        main.cpp
)

set_target_properties(KeyWave
        PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}
)

target_link_libraries(KeyWave
        PRIVATE
        keywave_app
)
