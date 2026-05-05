#include "render_opengl/OpenGLRendererBackend.hpp"

#include <glad/glad.h>

#include <iostream>

OpenGLRendererBackend::OpenGLRendererBackend(NativeProcAddressLoader procAddressLoader, Color clearColor)
    : procAddressLoader_(procAddressLoader),
      clearColor_(clearColor) {
}

OpenGLRendererBackend::~OpenGLRendererBackend() {
    shutdown();
}

bool OpenGLRendererBackend::initialize() {
    if (initialized_) {
        return true;
    }

    if (procAddressLoader_ == nullptr) {
        std::cerr << "OpenGL renderer initialization failed: missing OpenGL procedure loader.\n";
        return false;
    }

    if (gladLoadGLLoader(reinterpret_cast<GLADloadproc>(procAddressLoader_)) == 0) {
        std::cerr << "OpenGL renderer initialization failed: GLAD could not load OpenGL functions.\n";
        return false;
    }

    glViewport(0, 0, 1280, 720);

    initialized_ = true;
    return true;
}

void OpenGLRendererBackend::shutdown() {
    initialized_ = false;
}

void OpenGLRendererBackend::beginFrame() {
    glClearColor(clearColor_.r, clearColor_.g, clearColor_.b, clearColor_.a);
    glClear(GL_COLOR_BUFFER_BIT);
}

void OpenGLRendererBackend::endFrame() {
}
