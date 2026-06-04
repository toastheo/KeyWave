include(FetchContent)

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG 7b6aead9fb88b3623e3b3725ebb42670cbe4c579 # 3.4
    GIT_SHALLOW FALSE
)
FetchContent_MakeAvailable(glfw)

find_package(OpenGL REQUIRED)

add_library(glad STATIC
    ${PROJECT_SOURCE_DIR}/external/glad/src/glad.c
)

target_include_directories(glad
    PUBLIC
        ${PROJECT_SOURCE_DIR}/external/glad/include
)

add_library(midifile STATIC
    ${PROJECT_SOURCE_DIR}/external/midifile/src/Binasc.cpp
    ${PROJECT_SOURCE_DIR}/external/midifile/src/MidiEvent.cpp
    ${PROJECT_SOURCE_DIR}/external/midifile/src/MidiEventList.cpp
    ${PROJECT_SOURCE_DIR}/external/midifile/src/MidiFile.cpp
    ${PROJECT_SOURCE_DIR}/external/midifile/src/MidiMessage.cpp
)

target_include_directories(midifile
    PUBLIC
        ${PROJECT_SOURCE_DIR}/external/midifile/include
)

add_library(imgui STATIC
    ${PROJECT_SOURCE_DIR}/external/imgui/imgui.cpp
    ${PROJECT_SOURCE_DIR}/external/imgui/imgui_draw.cpp
    ${PROJECT_SOURCE_DIR}/external/imgui/imgui_tables.cpp
    ${PROJECT_SOURCE_DIR}/external/imgui/imgui_widgets.cpp
    ${PROJECT_SOURCE_DIR}/external/imgui/backends/imgui_impl_glfw.cpp
    ${PROJECT_SOURCE_DIR}/external/imgui/backends/imgui_impl_opengl3.cpp
)

target_include_directories(imgui
    PUBLIC
        ${PROJECT_SOURCE_DIR}/external/imgui
        ${PROJECT_SOURCE_DIR}/external/imgui/backends
)

target_compile_definitions(imgui
    PRIVATE
        GLFW_INCLUDE_NONE
)

target_link_libraries(imgui
    PRIVATE
        glfw
)
