cmake_minimum_required(VERSION 3.24)

project(KeyWave LANGUAGES C CXX)

option(KEYWAVE_ENABLE_AUDIO "Enable FluidSynth piano audio playback" ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(cmake/Dependencies.cmake)

add_subdirectory(src)

include(CTest)

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()

