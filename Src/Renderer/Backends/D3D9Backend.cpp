#include "Renderer/RendererBackend.h"

#include "Core/Log.h"
#include "imgui_impl_dx9.h"

#include <d3d9.h>

namespace UniversalOverlay::Renderer
{
    namespace
    {
        bool InitializeD3D9(const RendererInitContext& context)
        {
            IDirect3DDevice9* device = reinterpret_cast<IDirect3DDevice9*>(context.deviceOrHdc);
            if (!device)
            {
                Log::Debug("D3D9 renderer init missing device.");
                return false;
            }

            if (!ImGui_ImplDX9_Init(device))
            {
                Log::Debug("ImGui_ImplDX9_Init failed.");
                return false;
            }

            return true;
        }

        void BeginFrameD3D9()
        {
            ImGui_ImplDX9_NewFrame();
        }

        void EndFrameD3D9(const RendererFrameContext&)
        {
            ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
        }

        void ShutdownD3D9()
        {
            ImGui_ImplDX9_Shutdown();
        }
    }

    const RendererBackendVTable& GetD3D9Backend()
    {
        static const RendererBackendVTable backend = {
            "Direct3D 9",
            GraphicsAPI::D3D9,
            InitializeD3D9,
            BeginFrameD3D9,
            EndFrameD3D9,
            ShutdownD3D9
        };
        return backend;
    }
}
