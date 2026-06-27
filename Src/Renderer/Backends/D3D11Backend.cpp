#include "Renderer/RendererBackend.h"

#include "Core/Log.h"
#include "imgui_impl_dx11.h"

#include <d3d11.h>

namespace UniversalOverlay::Renderer
{
    namespace
    {
        bool InitializeD3D11(const RendererInitContext& context)
        {
            ID3D11Device* device = reinterpret_cast<ID3D11Device*>(context.deviceOrHdc);
            ID3D11DeviceContext* deviceContext = reinterpret_cast<ID3D11DeviceContext*>(context.swapChainOrContext);

            if (!device || !deviceContext)
            {
                Log::Debug("D3D11 renderer init missing device or device context.");
                return false;
            }

            if (!ImGui_ImplDX11_Init(device, deviceContext))
            {
                Log::Debug("ImGui_ImplDX11_Init failed.");
                return false;
            }

            return true;
        }

        void BeginFrameD3D11()
        {
            ImGui_ImplDX11_NewFrame();
        }

        void EndFrameD3D11(const RendererFrameContext&)
        {
            ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
        }

        void ShutdownD3D11()
        {
            ImGui_ImplDX11_Shutdown();
        }
    }

    const RendererBackendVTable& GetD3D11Backend()
    {
        static const RendererBackendVTable backend = {
            "Direct3D 11",
            GraphicsAPI::D3D11,
            InitializeD3D11,
            BeginFrameD3D11,
            EndFrameD3D11,
            ShutdownD3D11
        };
        return backend;
    }
}
