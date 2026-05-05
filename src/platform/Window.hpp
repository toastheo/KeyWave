#pragma once

#include <string>

struct WindowConfig {
    std::string title;
    int width = 1280;
    int height = 720;
};

using NativeProcAddressLoader = void* (*)(const char* name);

class Window {
public:
    Window() = default;
    ~Window();

    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;

    bool initialize(const WindowConfig& config);
    void shutdown();

    bool shouldClose() const;
    void pollEvents();
    void swapBuffers();
    void* nativeHandle() const;
    NativeProcAddressLoader nativeProcAddressLoader() const;

private:
    void* handle_ = nullptr;
    bool ownsGlfw_ = false;
};
