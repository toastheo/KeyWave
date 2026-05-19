include(FetchContent)

FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v3.11.0
)
FetchContent_MakeAvailable(Catch2)

add_library(keywave_test_support STATIC
    midi/MidiFixtures.hpp
    midi/MidiFixtures.cpp
)

target_include_directories(keywave_test_support
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

add_executable(KeyWaveMidiTests
    app/AppConfigTests.cpp
    app/PlaybackTransportControlsTests.cpp
    ui/TransportControlsConfigTests.cpp
    ui/TransportTimeTests.cpp
    midi/MidiFileLoaderTests.cpp
    midi/MidiTimelineQueryTests.cpp
    keyboard/KeyboardStateTests.cpp
    keyboard/KeyboardGeometryTests.cpp
    keyboard/KeyboardLayoutTests.cpp
    keyboard/KeyboardRenderAdapterTests.cpp
    fallingnotes/FallingNotesLayoutTests.cpp
    fallingnotes/FallingNotesRenderAdapterTests.cpp
    fallingnotes/FallingNotesSceneBuilderTests.cpp
    playback/PlaybackTransportTests.cpp
    render/RendererViewTests.cpp
)

set_target_properties(KeyWaveMidiTests
    PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}
)

target_link_libraries(KeyWaveMidiTests
    PRIVATE
        Catch2::Catch2WithMain
        keywave_app_core
        keywave_fallingnotes
        keywave_midi
        keywave_playback
        keywave_render
        keywave_test_support
        keywave_ui
)

list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
include(Catch)
catch_discover_tests(KeyWaveMidiTests DISCOVERY_MODE PRE_TEST)
