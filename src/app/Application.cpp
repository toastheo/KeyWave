#include "app/Application.hpp"

#include "render/RenderTypes.hpp"
#include "render_opengl/OpenGLRendererBackend.hpp"

#include <iostream>
#include <memory>

Application::~Application() {
    shutdown();
}

bool Application::initialize() {
    const WindowConfig windowConfig{
        .title = "KeyWave",
        .width = 1280,
        .height = 720,
    };

    if (!window_.initialize(windowConfig)) {
        std::cerr << "Application initialization failed: window could not be created.\n";
        return false;
    }

    renderer_ = std::make_unique<OpenGLRendererBackend>(
        window_.nativeProcAddressLoader(),
        Color{0.025f, 0.03f, 0.04f, 1.0f}
    );

    if (!renderer_->initialize()) {
        std::cerr << "Application initialization failed: renderer could not be initialized.\n";
        renderer_.reset();
        window_.shutdown();
        return false;
    }

    initialized_ = true;
    return true;
}

void Application::run() {
    if (!initialized_) {
        return;
    }

    while (!window_.shouldClose()) {
        window_.pollEvents();
        renderer_->beginFrame();
        renderer_->endFrame();
        window_.swapBuffers();
    }
}

void Application::shutdown() {
    if (renderer_) {
        renderer_->shutdown();
        renderer_.reset();
    }

    window_.shutdown();
    initialized_ = false;
}
