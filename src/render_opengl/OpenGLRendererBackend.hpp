#pragma once

#include "platform/Window.hpp"
#include "render/RendererBackend.hpp"
#include "render/RenderTypes.hpp"

class OpenGLRendererBackend final : public RendererBackend {
public:
    explicit OpenGLRendererBackend(NativeProcAddressLoader procAddressLoader, Color clearColor);
    ~OpenGLRendererBackend() override;

    OpenGLRendererBackend(const OpenGLRendererBackend&) = delete;
    OpenGLRendererBackend& operator=(const OpenGLRendererBackend&) = delete;

    bool initialize() override;
    void shutdown() override;
    void beginFrame() override;
    void endFrame() override;

private:
    NativeProcAddressLoader procAddressLoader_ = nullptr;
    Color clearColor_;
    bool initialized_ = false;
};
