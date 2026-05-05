#pragma once

#include "platform/Window.hpp"
#include "render/RendererBackend.hpp"

#include <memory>

class Application {
public:
    Application() = default;
    ~Application();

    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;

    bool initialize();
    void run();
    void shutdown();

private:
    Window window_;
    std::unique_ptr<RendererBackend> renderer_;
    bool initialized_ = false;
};
