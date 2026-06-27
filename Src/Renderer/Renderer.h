#pragma once

#include <Windows.h>

#include "Renderer/RendererDiagnostics.h"
#include "Renderer/RendererTypes.h"

namespace UniversalOverlay
{
    namespace Renderer
    {
        // Initializes the corresponding ImGui backend.
        // For OpenGL: deviceOrHdc is HDC.
        // For D3D9: deviceOrHdc is IDirect3DDevice9*.
        // For D3D11: deviceOrHdc is ID3D11Device*, swapChainOrContext is ID3D11DeviceContext*.
        // For D3D12: deviceOrHdc is ID3D12Device*, swapChainOrContext is command queue, command list, descriptor heaps, etc. We will define a structured context or unpack it.
        bool Initialize(HWND hwnd, GraphicsAPI api, void* deviceOrHdc, void* swapChainOrContext = nullptr, void* extraContext = nullptr);
        
        void Shutdown();
        
        void BeginFrame();
        void EndFrame(void* commandList = nullptr);
        
        bool IsInitialized();
        void SetDrawCursor(bool enabled);
        const RendererDiagnostics& GetDiagnostics();
    }
}
