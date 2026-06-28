#include "Renderer/Renderer.h"

#include "Core/Log.h"
#include "Renderer/RendererBackend.h"
#include "Renderer/RendererDiagnostics.h"
#include "Ui/OverlayTheme.h"
#include "imgui.h"
#include "imgui_impl_win32.h"

namespace UniversalOverlay
{
    namespace Renderer
    {
        namespace
        {
            bool g_initialized = false;
            const RendererBackendVTable* g_backend = nullptr;
            RendererDiagnostics g_diagnostics;

            const RendererBackendVTable* FindBackend(GraphicsAPI api)
            {
                switch (api)
                {
                case GraphicsAPI::OpenGL3:
                    return &GetOpenGL3Backend();
                case GraphicsAPI::D3D9:
                    return &GetD3D9Backend();
                case GraphicsAPI::D3D11:
                    return &GetD3D11Backend();
                case GraphicsAPI::D3D12:
                    return &GetD3D12Backend();
                default:
                    return nullptr;
                }
            }

            const char* GraphicsApiName(GraphicsAPI api)
            {
                switch (api)
                {
                case GraphicsAPI::OpenGL3:
                    return "OpenGL 3";
                case GraphicsAPI::D3D9:
                    return "Direct3D 9";
                case GraphicsAPI::D3D11:
                    return "Direct3D 11";
                case GraphicsAPI::D3D12:
                    return "Direct3D 12";
                default:
                    return "Unknown";
                }
            }

            void RecordFailure(GraphicsAPI api, const char* message)
            {
                g_diagnostics.initialized = false;
                g_diagnostics.activeBackend = api;
                g_diagnostics.activeBackendName = GraphicsApiName(api);
                g_diagnostics.lastError = message ? message : "";
            }

            void ResetRendererState()
            {
                g_initialized = false;
                g_backend = nullptr;
                g_diagnostics.initialized = false;
            }
        }

        bool Initialize(HWND hwnd, GraphicsAPI api, void* deviceOrHdc, void* swapChainOrContext, void* extraContext)
        {
            if (g_initialized)
                return true;

            const RendererBackendVTable* backend = FindBackend(api);
            if (!backend || !backend->Initialize || !backend->BeginFrame || !backend->EndFrame || !backend->Shutdown)
            {
                RecordFailure(api, "No renderer backend is registered for the requested API.");
                Log::Debug("No renderer backend registered for API ID: %d", static_cast<int>(api));
                return false;
            }

            IMGUI_CHECKVERSION();
            ImGui::CreateContext();
            ImGuiIO& io = ImGui::GetIO();
            io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

            Ui::ApplyTheme(Ui::GetTheme());

            if (!ImGui_ImplWin32_Init(hwnd))
            {
                RecordFailure(api, "ImGui Win32 backend initialization failed.");
                Log::Debug("ImGui_ImplWin32_Init failed.");
                ImGui::DestroyContext();
                ResetRendererState();
                return false;
            }

            RendererInitContext context = {};
            context.hwnd = hwnd;
            context.api = api;
            context.deviceOrHdc = deviceOrHdc;
            context.swapChainOrContext = swapChainOrContext;
            context.extraContext = extraContext;

            if (!backend->Initialize(context))
            {
                RecordFailure(api, "Renderer backend initialization failed.");
                Log::Debug("Renderer backend initialization failed for API ID: %d", static_cast<int>(api));
                ImGui_ImplWin32_Shutdown();
                ImGui::DestroyContext();
                ResetRendererState();
                return false;
            }

            g_backend = backend;
            g_initialized = true;
            g_diagnostics.initialized = true;
            g_diagnostics.activeBackend = api;
            g_diagnostics.activeBackendName = backend->name;
            g_diagnostics.lastError.clear();

            Log::Debug("ImGui Renderer initialized for API ID: %d", static_cast<int>(api));
            return true;
        }

        void Shutdown()
        {
            if (!g_initialized)
                return;

            if (g_backend && g_backend->Shutdown)
                g_backend->Shutdown();

            ImGui_ImplWin32_Shutdown();
            ImGui::DestroyContext();

            ResetRendererState();
            Log::Debug("ImGui Renderer shut down.");
        }

        void BeginFrame()
        {
            if (!g_initialized || !g_backend)
                return;

            g_backend->BeginFrame();
            ImGui_ImplWin32_NewFrame();
            ImGui::NewFrame();
        }

        void EndFrame(void* commandList)
        {
            if (!g_initialized || !g_backend)
                return;

            ImGui::Render();

            RendererFrameContext context = {};
            context.commandList = commandList;
            g_backend->EndFrame(context);
        }

        bool IsInitialized()
        {
            return g_initialized;
        }

        void SetDrawCursor(bool enabled)
        {
            if (!ImGui::GetCurrentContext())
                return;

            ImGuiIO& io = ImGui::GetIO();
            io.MouseDrawCursor = enabled;
        }

        const RendererDiagnostics& GetDiagnostics()
        {
            return g_diagnostics;
        }
    }
}
