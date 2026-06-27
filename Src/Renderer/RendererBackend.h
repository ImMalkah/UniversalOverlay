#pragma once

#include "Renderer/RendererTypes.h"

#include <Windows.h>

namespace UniversalOverlay::Renderer
{
    struct RendererInitContext
    {
        HWND hwnd = nullptr;
        GraphicsAPI api = GraphicsAPI::OpenGL3;
        void* deviceOrHdc = nullptr;
        void* swapChainOrContext = nullptr;
        void* extraContext = nullptr;
    };

    struct RendererFrameContext
    {
        void* commandList = nullptr;
    };

    struct RendererBackendVTable
    {
        const char* name = "";
        GraphicsAPI api = GraphicsAPI::OpenGL3;
        bool (*Initialize)(const RendererInitContext& context) = nullptr;
        void (*BeginFrame)() = nullptr;
        void (*EndFrame)(const RendererFrameContext& context) = nullptr;
        void (*Shutdown)() = nullptr;
    };

    const RendererBackendVTable& GetOpenGL3Backend();
    const RendererBackendVTable& GetD3D9Backend();
    const RendererBackendVTable& GetD3D10Backend();
    const RendererBackendVTable& GetD3D11Backend();
    const RendererBackendVTable& GetD3D12Backend();
}
