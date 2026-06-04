#include "Renderer.h"
#include "CoreState.h"
#include "Log.h"

#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_opengl3.h"
#include "imgui_impl_dx9.h"
#include "imgui_impl_dx11.h"
#include "imgui_impl_dx12.h"

#include <d3d9.h>
#include <d3d11.h>
#include <d3d12.h>
#include <dxgi1_4.h>

namespace UniversalOverlay
{
    namespace Renderer
    {
        static bool g_initialized = false;
        static GraphicsAPI g_api = GraphicsAPI::OpenGL3;
        static HWND g_hwnd = nullptr;

        // DX12 specific resources
        static ID3D12DescriptorHeap* g_pd3d12DescriptorHeap = nullptr;

        bool Initialize(HWND hwnd, GraphicsAPI api, void* deviceOrHdc, void* swapChainOrContext)
        {
            if (g_initialized)
                return true;

            g_hwnd = hwnd;
            g_api = api;

            IMGUI_CHECKVERSION();
            ImGui::CreateContext();
            ImGuiIO& io = ImGui::GetIO();
            io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

            ImGui::StyleColorsDark();

            // Set up a modern visual style (Curated Slate/Purple theme)
            ImGuiStyle& style = ImGui::GetStyle();
            style.WindowRounding = 6.0f;
            style.ChildRounding = 4.0f;
            style.FrameRounding = 4.0f;
            style.PopupRounding = 4.0f;
            style.ScrollbarRounding = 4.0f;
            style.GrabRounding = 4.0f;
            style.TabRounding = 4.0f;
            
            // Subtle premium dark color palette
            ImVec4* colors = style.Colors;
            colors[ImGuiCol_Text]                   = ImVec4(0.95f, 0.96f, 0.98f, 1.00f);
            colors[ImGuiCol_TextDisabled]           = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
            colors[ImGuiCol_WindowBg]               = ImVec4(0.10f, 0.10f, 0.12f, 0.95f);
            colors[ImGuiCol_ChildBg]                = ImVec4(0.14f, 0.14f, 0.16f, 1.00f);
            colors[ImGuiCol_PopupBg]                = ImVec4(0.12f, 0.12f, 0.14f, 0.95f);
            colors[ImGuiCol_Border]                 = ImVec4(0.25f, 0.25f, 0.28f, 1.00f);
            colors[ImGuiCol_BorderShadow]           = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
            colors[ImGuiCol_FrameBg]                = ImVec4(0.20f, 0.20f, 0.22f, 1.00f);
            colors[ImGuiCol_FrameBgHovered]         = ImVec4(0.25f, 0.25f, 0.28f, 1.00f);
            colors[ImGuiCol_FrameBgActive]          = ImVec4(0.30f, 0.30f, 0.34f, 1.00f);
            colors[ImGuiCol_TitleBg]                = ImVec4(0.15f, 0.12f, 0.22f, 1.00f); // Purple hue
            colors[ImGuiCol_TitleBgActive]          = ImVec4(0.22f, 0.15f, 0.35f, 1.00f);
            colors[ImGuiCol_TitleBgCollapsed]       = ImVec4(0.15f, 0.12f, 0.22f, 1.00f);
            colors[ImGuiCol_MenuBarBg]              = ImVec4(0.14f, 0.14f, 0.16f, 1.00f);
            colors[ImGuiCol_ScrollbarBg]            = ImVec4(0.02f, 0.02f, 0.02f, 0.39f);
            colors[ImGuiCol_ScrollbarGrab]          = ImVec4(0.31f, 0.31f, 0.35f, 1.00f);
            colors[ImGuiCol_ScrollbarGrabHovered]   = ImVec4(0.41f, 0.41f, 0.45f, 1.00f);
            colors[ImGuiCol_ScrollbarGrabActive]    = ImVec4(0.51f, 0.51f, 0.55f, 1.00f);
            colors[ImGuiCol_CheckMark]              = ImVec4(0.65f, 0.40f, 0.95f, 1.00f);
            colors[ImGuiCol_SliderGrab]             = ImVec4(0.65f, 0.40f, 0.95f, 1.00f);
            colors[ImGuiCol_SliderGrabActive]       = ImVec4(0.75f, 0.50f, 1.00f, 1.00f);
            colors[ImGuiCol_Button]                 = ImVec4(0.25f, 0.22f, 0.35f, 1.00f);
            colors[ImGuiCol_ButtonHovered]          = ImVec4(0.35f, 0.30f, 0.50f, 1.00f);
            colors[ImGuiCol_ButtonActive]           = ImVec4(0.45f, 0.35f, 0.65f, 1.00f);
            colors[ImGuiCol_Header]                 = ImVec4(0.30f, 0.25f, 0.45f, 1.00f);
            colors[ImGuiCol_HeaderHovered]          = ImVec4(0.40f, 0.35f, 0.55f, 1.00f);
            colors[ImGuiCol_HeaderActive]           = ImVec4(0.50f, 0.42f, 0.65f, 1.00f);
            colors[ImGuiCol_Separator]              = ImVec4(0.25f, 0.25f, 0.28f, 1.00f);
            colors[ImGuiCol_SeparatorHovered]       = ImVec4(0.40f, 0.35f, 0.55f, 1.00f);
            colors[ImGuiCol_SeparatorActive]        = ImVec4(0.50f, 0.42f, 0.65f, 1.00f);
            colors[ImGuiCol_Tab]                    = ImVec4(0.18f, 0.18f, 0.20f, 1.00f);
            colors[ImGuiCol_TabHovered]             = ImVec4(0.35f, 0.30f, 0.50f, 1.00f);
            colors[ImGuiCol_TabActive]              = ImVec4(0.30f, 0.25f, 0.45f, 1.00f);

            if (!ImGui_ImplWin32_Init(hwnd))
            {
                Log::Debug("ImGui_ImplWin32_Init failed.");
                return false;
            }

            switch (api)
            {
            case GraphicsAPI::OpenGL3:
                if (!ImGui_ImplOpenGL3_Init("#version 130"))
                {
                    Log::Debug("ImGui_ImplOpenGL3_Init failed.");
                    ImGui_ImplWin32_Shutdown();
                    return false;
                }
                break;

            case GraphicsAPI::D3D9:
                if (!ImGui_ImplDX9_Init(reinterpret_cast<IDirect3DDevice9*>(deviceOrHdc)))
                {
                    Log::Debug("ImGui_ImplDX9_Init failed.");
                    ImGui_ImplWin32_Shutdown();
                    return false;
                }
                break;

            case GraphicsAPI::D3D11:
                if (!ImGui_ImplDX11_Init(
                    reinterpret_cast<ID3D11Device*>(deviceOrHdc),
                    reinterpret_cast<ID3D11DeviceContext*>(swapChainOrContext)))
                {
                    Log::Debug("ImGui_ImplDX11_Init failed.");
                    ImGui_ImplWin32_Shutdown();
                    return false;
                }
                break;

            case GraphicsAPI::D3D12:
                {
                    ID3D12Device* device = reinterpret_cast<ID3D12Device*>(deviceOrHdc);
                    IDXGISwapChain3* swapChain = reinterpret_cast<IDXGISwapChain3*>(swapChainOrContext);

                    // Create Descriptor Heap for SRV (fonts texture)
                    D3D12_DESCRIPTOR_HEAP_DESC desc = {};
                    desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
                    desc.NumDescriptors = 1;
                    desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

                    if (FAILED(device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&g_pd3d12DescriptorHeap))))
                    {
                        Log::Debug("Failed to create DX12 descriptor heap for ImGui.");
                        ImGui_ImplWin32_Shutdown();
                        return false;
                    }

                    // Query swap chain properties to configure backbuffers
                    DXGI_SWAP_CHAIN_DESC swapChainDesc;
                    swapChain->GetDesc(&swapChainDesc);

                    if (!ImGui_ImplDX12_Init(
                        device,
                        swapChainDesc.BufferCount,
                        swapChainDesc.BufferDesc.Format,
                        g_pd3d12DescriptorHeap,
                        g_pd3d12DescriptorHeap->GetCPUDescriptorHandleForHeapStart(),
                        g_pd3d12DescriptorHeap->GetGPUDescriptorHandleForHeapStart()))
                    {
                        Log::Debug("ImGui_ImplDX12_Init failed.");
                        g_pd3d12DescriptorHeap->Release();
                        g_pd3d12DescriptorHeap = nullptr;
                        ImGui_ImplWin32_Shutdown();
                        return false;
                    }
                }
                break;
            }

            g_initialized = true;
            Log::Debug("ImGui Renderer initialized for API ID: %d", static_cast<int>(api));
            return true;
        }

        void Shutdown()
        {
            if (!g_initialized)
                return;

            switch (g_api)
            {
            case GraphicsAPI::OpenGL3:
                ImGui_ImplOpenGL3_Shutdown();
                break;
            case GraphicsAPI::D3D9:
                ImGui_ImplDX9_Shutdown();
                break;
            case GraphicsAPI::D3D11:
                ImGui_ImplDX11_Shutdown();
                break;
            case GraphicsAPI::D3D12:
                ImGui_ImplDX12_Shutdown();
                if (g_pd3d12DescriptorHeap)
                {
                    g_pd3d12DescriptorHeap->Release();
                    g_pd3d12DescriptorHeap = nullptr;
                }
                break;
            }

            ImGui_ImplWin32_Shutdown();
            ImGui::DestroyContext();

            g_initialized = false;
            Log::Debug("ImGui Renderer shut down.");
        }

        void BeginFrame()
        {
            if (!g_initialized)
                return;

            switch (g_api)
            {
            case GraphicsAPI::OpenGL3:
                ImGui_ImplOpenGL3_NewFrame();
                break;
            case GraphicsAPI::D3D9:
                ImGui_ImplDX9_NewFrame();
                break;
            case GraphicsAPI::D3D11:
                ImGui_ImplDX11_NewFrame();
                break;
            case GraphicsAPI::D3D12:
                ImGui_ImplDX12_NewFrame();
                break;
            }

            ImGui_ImplWin32_NewFrame();
            ImGui::NewFrame();
        }

        void EndFrame(void* commandList)
        {
            if (!g_initialized)
                return;

            ImGui::Render();

            switch (g_api)
            {
            case GraphicsAPI::OpenGL3:
                ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
                break;
            case GraphicsAPI::D3D9:
                ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
                break;
            case GraphicsAPI::D3D11:
                ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
                break;
            case GraphicsAPI::D3D12:
                if (commandList)
                {
                    ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), reinterpret_cast<ID3D12GraphicsCommandList*>(commandList));
                }
                break;
            }
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
    }
}
