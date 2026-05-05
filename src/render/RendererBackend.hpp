#pragma once

class RendererBackend {
public:
    virtual ~RendererBackend() = default;

    virtual bool initialize() = 0;
    virtual void shutdown() = 0;
    virtual void beginFrame() = 0;
    virtual void endFrame() = 0;
};
