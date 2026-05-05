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

target_link_libraries(KeyWave
    PRIVATE
        glad
        glfw
        OpenGL::GL
)
