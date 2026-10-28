cmake_minimum_required(VERSION 3.24)

project(KeyWave LANGUAGES C CXX)

option(KEYWAVE_ENABLE_AUDIO "Enable FluidSynth piano audio playback" ON)
option(KEYWAVE_ENABLE_GPU_TESTS "Build integration tests requiring an OpenGL 3.3 context" OFF)
option(KEYWAVE_ENABLE_ENCODER_TESTS "Build integration tests requiring FFmpeg with libx264" OFF)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(cmake/Dependencies.cmake)

add_subdirectory(src)

include(CTest)

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
