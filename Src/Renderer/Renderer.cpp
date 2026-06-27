#include "Renderer/Renderer.h"

#include "Core/Log.h"
#include "Renderer/RendererBackend.h"
#include "Renderer/RendererDiagnostics.h"
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
                case GraphicsAPI::D3D10:
                    return &GetD3D10Backend();
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
                case GraphicsAPI::D3D10:
                    return "Direct3D 10";
                case GraphicsAPI::D3D11:
                    return "Direct3D 11";
                case GraphicsAPI::D3D12:
                    return "Direct3D 12";
                default:
                    return "Unknown";
                }
            }

            void ApplyDefaultStyle()
            {
                ImGui::StyleColorsDark();

                ImGuiStyle& style = ImGui::GetStyle();
                style.WindowRounding = 6.0f;
                style.ChildRounding = 4.0f;
                style.FrameRounding = 4.0f;
                style.PopupRounding = 4.0f;
                style.ScrollbarRounding = 4.0f;
                style.GrabRounding = 4.0f;
                style.TabRounding = 4.0f;

                ImVec4* colors = style.Colors;
                colors[ImGuiCol_Text]                 = ImVec4(0.95f, 0.96f, 0.98f, 1.00f);
                colors[ImGuiCol_TextDisabled]         = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
                colors[ImGuiCol_WindowBg]             = ImVec4(0.10f, 0.10f, 0.12f, 0.95f);
                colors[ImGuiCol_ChildBg]              = ImVec4(0.14f, 0.14f, 0.16f, 1.00f);
                colors[ImGuiCol_PopupBg]              = ImVec4(0.12f, 0.12f, 0.14f, 0.95f);
                colors[ImGuiCol_Border]               = ImVec4(0.25f, 0.25f, 0.28f, 1.00f);
                colors[ImGuiCol_BorderShadow]         = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
                colors[ImGuiCol_FrameBg]              = ImVec4(0.20f, 0.20f, 0.22f, 1.00f);
                colors[ImGuiCol_FrameBgHovered]       = ImVec4(0.25f, 0.25f, 0.28f, 1.00f);
                colors[ImGuiCol_FrameBgActive]        = ImVec4(0.30f, 0.30f, 0.34f, 1.00f);
                colors[ImGuiCol_TitleBg]              = ImVec4(0.15f, 0.12f, 0.22f, 1.00f);
                colors[ImGuiCol_TitleBgActive]        = ImVec4(0.22f, 0.15f, 0.35f, 1.00f);
                colors[ImGuiCol_TitleBgCollapsed]     = ImVec4(0.15f, 0.12f, 0.22f, 1.00f);
                colors[ImGuiCol_MenuBarBg]            = ImVec4(0.14f, 0.14f, 0.16f, 1.00f);
                colors[ImGuiCol_ScrollbarBg]          = ImVec4(0.02f, 0.02f, 0.02f, 0.39f);
                colors[ImGuiCol_ScrollbarGrab]        = ImVec4(0.31f, 0.31f, 0.35f, 1.00f);
                colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.41f, 0.41f, 0.45f, 1.00f);
                colors[ImGuiCol_ScrollbarGrabActive]  = ImVec4(0.51f, 0.51f, 0.55f, 1.00f);
                colors[ImGuiCol_CheckMark]            = ImVec4(0.65f, 0.40f, 0.95f, 1.00f);
                colors[ImGuiCol_SliderGrab]           = ImVec4(0.65f, 0.40f, 0.95f, 1.00f);
                colors[ImGuiCol_SliderGrabActive]     = ImVec4(0.75f, 0.50f, 1.00f, 1.00f);
                colors[ImGuiCol_Button]               = ImVec4(0.25f, 0.22f, 0.35f, 1.00f);
                colors[ImGuiCol_ButtonHovered]        = ImVec4(0.35f, 0.30f, 0.50f, 1.00f);
                colors[ImGuiCol_ButtonActive]         = ImVec4(0.45f, 0.35f, 0.65f, 1.00f);
                colors[ImGuiCol_Header]               = ImVec4(0.30f, 0.25f, 0.45f, 1.00f);
                colors[ImGuiCol_HeaderHovered]        = ImVec4(0.40f, 0.35f, 0.55f, 1.00f);
                colors[ImGuiCol_HeaderActive]         = ImVec4(0.50f, 0.42f, 0.65f, 1.00f);
                colors[ImGuiCol_Separator]            = ImVec4(0.25f, 0.25f, 0.28f, 1.00f);
                colors[ImGuiCol_SeparatorHovered]     = ImVec4(0.40f, 0.35f, 0.55f, 1.00f);
                colors[ImGuiCol_SeparatorActive]      = ImVec4(0.50f, 0.42f, 0.65f, 1.00f);
                colors[ImGuiCol_Tab]                  = ImVec4(0.18f, 0.18f, 0.20f, 1.00f);
                colors[ImGuiCol_TabHovered]           = ImVec4(0.35f, 0.30f, 0.50f, 1.00f);
                colors[ImGuiCol_TabActive]            = ImVec4(0.30f, 0.25f, 0.45f, 1.00f);
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

            ApplyDefaultStyle();

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
