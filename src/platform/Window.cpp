#include "platform/Window.hpp"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <iostream>

namespace {
void printGlfwError(const char* fallbackMessage) {
    const char* description = nullptr;
    const int errorCode = glfwGetError(&description);

    if (description != nullptr) {
        std::cerr << fallbackMessage << " GLFW error " << errorCode << ": " << description << '\n';
    } else {
        std::cerr << fallbackMessage << '\n';
    }
}

void* loadOpenGLProcAddress(const char* name) {
    return reinterpret_cast<void*>(glfwGetProcAddress(name));
}
}

Window::~Window() {
    shutdown();
}

bool Window::initialize(const WindowConfig& config) {
    if (handle_ != nullptr) {
        return true;
    }

    if (glfwInit() != GLFW_TRUE) {
        printGlfwError("Failed to initialize GLFW.");
        return false;
    }
    ownsGlfw_ = true;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#if defined(__APPLE__)
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
#endif

    GLFWwindow* window = glfwCreateWindow(config.width, config.height, config.title.c_str(), nullptr, nullptr);
    if (window == nullptr) {
        printGlfwError("Failed to create the KeyWave window.");
        shutdown();
        return false;
    }

    handle_ = window;
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    return true;
}

void Window::shutdown() {
    if (handle_ != nullptr) {
        glfwDestroyWindow(static_cast<GLFWwindow*>(handle_));
        handle_ = nullptr;
    }

    if (ownsGlfw_) {
        glfwTerminate();
        ownsGlfw_ = false;
    }
}

bool Window::shouldClose() const {
    return handle_ == nullptr || glfwWindowShouldClose(static_cast<GLFWwindow*>(handle_)) == GLFW_TRUE;
}

void Window::pollEvents() {
    glfwPollEvents();
}

void Window::swapBuffers() {
    if (handle_ != nullptr) {
        glfwSwapBuffers(static_cast<GLFWwindow*>(handle_));
    }
}

void* Window::nativeHandle() const {
    return handle_;
}

NativeProcAddressLoader Window::nativeProcAddressLoader() const {
    return loadOpenGLProcAddress;
}
