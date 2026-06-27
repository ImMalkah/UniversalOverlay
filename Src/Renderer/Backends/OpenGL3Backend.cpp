#include "Renderer/RendererBackend.h"

#include "Core/Log.h"
#include "imgui_impl_opengl3.h"

namespace UniversalOverlay::Renderer
{
    namespace
    {
        bool InitializeOpenGL3(const RendererInitContext&)
        {
            if (!ImGui_ImplOpenGL3_Init("#version 130"))
            {
                Log::Debug("ImGui_ImplOpenGL3_Init failed.");
                return false;
            }

            return true;
        }

        void BeginFrameOpenGL3()
        {
            ImGui_ImplOpenGL3_NewFrame();
        }

        void EndFrameOpenGL3(const RendererFrameContext&)
        {
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        }

        void ShutdownOpenGL3()
        {
            ImGui_ImplOpenGL3_Shutdown();
        }
    }

    const RendererBackendVTable& GetOpenGL3Backend()
    {
        static const RendererBackendVTable backend = {
            "OpenGL 3",
            GraphicsAPI::OpenGL3,
            InitializeOpenGL3,
            BeginFrameOpenGL3,
            EndFrameOpenGL3,
            ShutdownOpenGL3
        };
        return backend;
    }
}
