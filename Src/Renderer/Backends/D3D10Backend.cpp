#include "Renderer/RendererBackend.h"

#include "Core/OverlayLog.h"

namespace UniversalOverlay::Renderer
{
    namespace
    {
        bool InitializeD3D10(const RendererInitContext&)
        {
            Log::Warn(
                Log::LogCategory::Renderer,
                "Direct3D 10 backend is registered but not runtime-enabled yet");
            return false;
        }

        void BeginFrameD3D10()
        {
        }

        void EndFrameD3D10(const RendererFrameContext&)
        {
        }

        void ShutdownD3D10()
        {
        }
    }

    const RendererBackendVTable& GetD3D10Backend()
    {
        static const RendererBackendVTable backend = {
            "Direct3D 10",
            GraphicsAPI::D3D10,
            InitializeD3D10,
            BeginFrameD3D10,
            EndFrameD3D10,
            ShutdownD3D10
        };
        return backend;
    }
}
