include(FetchContent)

FetchContent_Declare(
        Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG 6ee0826dcae55ed1e06b2c5701981221e979e1e6 # v3.15.0
        GIT_SHALLOW FALSE
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
        architecture/LayeringTests.cpp
        architecture/SpanApiTests.cpp
        app/AppConfigTests.cpp
        app/AppSettingsTests.cpp
        app/AppSettingsSerializerTests.cpp
        app/PlaybackTransportControlsTests.cpp
        app/SettingsStorageTests.cpp
        app/VisualizerControllerTests.cpp
        app/VisualizationSettingsAdaptersTests.cpp
        app/VisualizationSettingsPanelControlsTests.cpp
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
        fallingnotes/PianoRollSceneBuilderTests.cpp
        playback/PlaybackTransportTests.cpp
        platform/WindowStateTests.cpp
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

target_compile_definitions(KeyWaveMidiTests
        PRIVATE
        KEYWAVE_SOURCE_DIR="${PROJECT_SOURCE_DIR}"
)

list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
include(Catch)

set(KEYWAVE_CATCH_DISCOVERY_ARGS DISCOVERY_MODE PRE_TEST)
if (WIN32)
    get_filename_component(KEYWAVE_TEST_RUNTIME_DIR "${CMAKE_CXX_COMPILER}" DIRECTORY)
    list(APPEND KEYWAVE_CATCH_DISCOVERY_ARGS DL_PATHS "${KEYWAVE_TEST_RUNTIME_DIR}")
endif ()

catch_discover_tests(KeyWaveMidiTests ${KEYWAVE_CATCH_DISCOVERY_ARGS})
