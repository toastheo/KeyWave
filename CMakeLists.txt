cmake_minimum_required(VERSION 3.24)

project(KeyWave LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG 3.4
)
FetchContent_MakeAvailable(glfw)

FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v3.11.0
)
FetchContent_MakeAvailable(Catch2)

find_package(OpenGL REQUIRED)

add_library(glad STATIC
    external/glad/src/glad.c
)

target_include_directories(glad
    PUBLIC
        external/glad/include
)

add_library(midifile STATIC
    external/midifile/src/Binasc.cpp
    external/midifile/src/MidiEvent.cpp
    external/midifile/src/MidiEventList.cpp
    external/midifile/src/MidiFile.cpp
    external/midifile/src/MidiMessage.cpp
)

target_include_directories(midifile
    PUBLIC
        external/midifile/include
)

add_library(imgui STATIC
    external/imgui/imgui.cpp
    external/imgui/imgui_draw.cpp
    external/imgui/imgui_tables.cpp
    external/imgui/imgui_widgets.cpp
    external/imgui/backends/imgui_impl_glfw.cpp
    external/imgui/backends/imgui_impl_opengl3.cpp
)

target_include_directories(imgui
    PUBLIC
        external/imgui
        external/imgui/backends
)

target_compile_definitions(imgui
    PRIVATE
        GLFW_INCLUDE_NONE
)

target_link_libraries(imgui
    PRIVATE
        glfw
)

add_library(keywave_midi STATIC
    src/midi/MidiTypes.hpp
    src/midi/MidiTrack.hpp
    src/midi/MidiTrack.cpp
    src/midi/MidiTimeline.hpp
    src/midi/MidiTimeline.cpp
    src/midi/MidiTimelineQuery.hpp
    src/midi/MidiTimelineQuery.cpp
    src/midi/MidiFileLoader.hpp
    src/midi/MidiFileLoader.cpp
)

target_include_directories(keywave_midi
    PUBLIC
        src
)

target_link_libraries(keywave_midi
    PRIVATE
        midifile
)

add_library(keywave_keyboard STATIC
    src/keyboard/KeyboardTypes.hpp
    src/keyboard/KeyboardState.hpp
    src/keyboard/KeyboardState.cpp
    src/keyboard/KeyboardGeometry.hpp
    src/keyboard/KeyboardGeometry.cpp
    src/keyboard/KeyboardLayout.hpp
    src/keyboard/KeyboardLayout.cpp
    src/keyboard/KeyboardRenderAdapter.hpp
    src/keyboard/KeyboardRenderAdapter.cpp
)

target_include_directories(keywave_keyboard
    PUBLIC
        src
)

target_link_libraries(keywave_keyboard
    PUBLIC
        keywave_midi
)

add_library(keywave_fallingnotes STATIC
    src/fallingnotes/FallingNotesTypes.hpp
    src/fallingnotes/FallingNotesLayout.hpp
    src/fallingnotes/FallingNotesLayout.cpp
    src/fallingnotes/FallingNotesRenderAdapter.hpp
    src/fallingnotes/FallingNotesRenderAdapter.cpp
    src/fallingnotes/FallingNotesSceneBuilder.hpp
    src/fallingnotes/FallingNotesSceneBuilder.cpp
)

target_include_directories(keywave_fallingnotes
    PUBLIC
        src
)

target_link_libraries(keywave_fallingnotes
    PUBLIC
        keywave_keyboard
        keywave_midi
)

add_library(keywave_playback STATIC
    src/playback/PlaybackTransport.hpp
    src/playback/PlaybackTransport.cpp
)

target_include_directories(keywave_playback
    PUBLIC
        src
)

add_executable(KeyWave
    src/main.cpp
    src/app/AppConfig.hpp
    src/app/AppConfig.cpp
    src/app/Application.hpp
    src/app/Application.cpp
    src/app/PlaybackTransportAction.hpp
    src/app/PlaybackTransportAction.cpp
    src/app/PlaybackTransportControls.hpp
    src/app/PlaybackTransportControls.cpp
    src/app/StartupDataLoader.hpp
    src/app/StartupDataLoader.cpp
    src/input/Key.hpp
    src/platform/Window.hpp
    src/platform/Window.cpp
    src/render/RendererBackend.hpp
    src/render/RenderCommand.hpp
    src/render/RenderScene.hpp
    src/render/RenderTypes.hpp
    src/render/RendererView.hpp
    src/render/RendererView.cpp
    src/render_opengl/OpenGLRendererBackend.hpp
    src/render_opengl/OpenGLRendererBackend.cpp
    src/render_opengl/OpenGLShader.hpp
    src/render_opengl/OpenGLShader.cpp
    src/ui/ImGuiLayer.hpp
    src/ui/ImGuiLayer.cpp
    src/ui/TransportControls.hpp
    src/ui/TransportControls.cpp
    src/ui/TransportControlsConfig.hpp
    src/ui/TransportControlsConfig.cpp
    src/ui/TransportTime.hpp
    src/ui/TransportTime.cpp
)

target_include_directories(KeyWave
    PRIVATE
        src
)

target_link_libraries(KeyWave
    PRIVATE
        keywave_fallingnotes
        keywave_midi
        keywave_playback
        glad
        glfw
        imgui
        OpenGL::GL
)

enable_testing()

add_library(keywave_test_support STATIC
    tests/midi/MidiFixtures.hpp
    tests/midi/MidiFixtures.cpp
)

target_include_directories(keywave_test_support
    PUBLIC
        tests
)

add_executable(KeyWaveMidiTests
    tests/app/AppConfigTests.cpp
    tests/app/PlaybackTransportControlsTests.cpp
    tests/ui/TransportControlsConfigTests.cpp
    tests/ui/TransportTimeTests.cpp
    tests/midi/MidiFileLoaderTests.cpp
    tests/midi/MidiTimelineQueryTests.cpp
    tests/keyboard/KeyboardStateTests.cpp
    tests/keyboard/KeyboardGeometryTests.cpp
    tests/keyboard/KeyboardLayoutTests.cpp
    tests/keyboard/KeyboardRenderAdapterTests.cpp
    tests/fallingnotes/FallingNotesLayoutTests.cpp
    tests/fallingnotes/FallingNotesRenderAdapterTests.cpp
    tests/fallingnotes/FallingNotesSceneBuilderTests.cpp
    tests/playback/PlaybackTransportTests.cpp
    tests/render/RendererViewTests.cpp
    src/app/PlaybackTransportAction.hpp
    src/app/PlaybackTransportAction.cpp
    src/app/AppConfig.hpp
    src/app/AppConfig.cpp
    src/app/PlaybackTransportControls.hpp
    src/app/PlaybackTransportControls.cpp
    src/ui/TransportControlsConfig.hpp
    src/ui/TransportControlsConfig.cpp
    src/ui/TransportTime.hpp
    src/ui/TransportTime.cpp
    src/input/Key.hpp
    src/render/RendererView.cpp
)

target_link_libraries(KeyWaveMidiTests
    PRIVATE
        Catch2::Catch2WithMain
        keywave_fallingnotes
        keywave_midi
        keywave_playback
        keywave_test_support
)

target_include_directories(KeyWaveMidiTests
    PRIVATE
        external/imgui
)

list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
include(Catch)
catch_discover_tests(KeyWaveMidiTests DISCOVERY_MODE PRE_TEST)
