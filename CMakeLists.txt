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

add_library(keywave_midi STATIC
    src/midi/MidiTypes.hpp
    src/midi/MidiTrack.hpp
    src/midi/MidiTrack.cpp
    src/midi/MidiTimeline.hpp
    src/midi/MidiTimeline.cpp
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

add_executable(KeyWave
    src/main.cpp
    src/app/Application.hpp
    src/app/Application.cpp
    src/platform/Window.hpp
    src/platform/Window.cpp
    src/render/RendererBackend.hpp
    src/render/RenderTypes.hpp
    src/render_opengl/OpenGLRendererBackend.hpp
    src/render_opengl/OpenGLRendererBackend.cpp
)

target_include_directories(KeyWave
    PRIVATE
        src
)

target_compile_definitions(KeyWave
    PRIVATE
        KEYWAVE_SOURCE_DIR="${CMAKE_SOURCE_DIR}"
)

target_link_libraries(KeyWave
    PRIVATE
        keywave_midi
        glad
        glfw
        OpenGL::GL
)

enable_testing()

add_executable(KeyWaveMidiTests
    tests/MidiFileLoaderTests.cpp
)

target_link_libraries(KeyWaveMidiTests
    PRIVATE
        keywave_midi
)

add_test(NAME KeyWaveMidiTests COMMAND KeyWaveMidiTests)
