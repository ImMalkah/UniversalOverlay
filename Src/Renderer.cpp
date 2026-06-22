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

#include <array>

namespace UniversalOverlay
{
    namespace Renderer
    {
        static bool g_initialized = false;
        static GraphicsAPI g_api = GraphicsAPI::OpenGL3;
        static HWND g_hwnd = nullptr;

        // DX12 specific resources
        constexpr UINT kDx12SrvDescriptorCount = 64;
        static ID3D12DescriptorHeap* g_pd3d12DescriptorHeap = nullptr;
        static UINT g_dx12SrvDescriptorSize = 0;
        static std::array<bool, kDx12SrvDescriptorCount> g_dx12SrvDescriptorUsed = {};

        static void AllocateDx12SrvDescriptor(ImGui_ImplDX12_InitInfo*, D3D12_CPU_DESCRIPTOR_HANDLE* outCpuHandle, D3D12_GPU_DESCRIPTOR_HANDLE* outGpuHandle)
        {
            if (!outCpuHandle || !outGpuHandle || !g_pd3d12DescriptorHeap || g_dx12SrvDescriptorSize == 0)
                return;

            D3D12_CPU_DESCRIPTOR_HANDLE cpuStart = g_pd3d12DescriptorHeap->GetCPUDescriptorHandleForHeapStart();
            D3D12_GPU_DESCRIPTOR_HANDLE gpuStart = g_pd3d12DescriptorHeap->GetGPUDescriptorHandleForHeapStart();

            for (UINT i = 0; i < kDx12SrvDescriptorCount; ++i)
            {
                if (g_dx12SrvDescriptorUsed[i])
                    continue;

                g_dx12SrvDescriptorUsed[i] = true;
                outCpuHandle->ptr = cpuStart.ptr + static_cast<SIZE_T>(i) * g_dx12SrvDescriptorSize;
                outGpuHandle->ptr = gpuStart.ptr + static_cast<UINT64>(i) * g_dx12SrvDescriptorSize;
                return;
            }

            Log::Debug("DX12 ImGui SRV descriptor heap exhausted.");
            outCpuHandle->ptr = 0;
            outGpuHandle->ptr = 0;
        }

        static void FreeDx12SrvDescriptor(ImGui_ImplDX12_InitInfo*, D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle, D3D12_GPU_DESCRIPTOR_HANDLE)
        {
            if (!g_pd3d12DescriptorHeap || g_dx12SrvDescriptorSize == 0 || cpuHandle.ptr == 0)
                return;

            D3D12_CPU_DESCRIPTOR_HANDLE cpuStart = g_pd3d12DescriptorHeap->GetCPUDescriptorHandleForHeapStart();
            if (cpuHandle.ptr < cpuStart.ptr)
                return;

            const SIZE_T offset = cpuHandle.ptr - cpuStart.ptr;
            if ((offset % g_dx12SrvDescriptorSize) != 0)
                return;

            const UINT index = static_cast<UINT>(offset / g_dx12SrvDescriptorSize);
            if (index < kDx12SrvDescriptorCount)
                g_dx12SrvDescriptorUsed[index] = false;
        }

        bool Initialize(HWND hwnd, GraphicsAPI api, void* deviceOrHdc, void* swapChainOrContext, void* extraContext)
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
                    IDXGISwapChain* swapChain = reinterpret_cast<IDXGISwapChain*>(swapChainOrContext);
                    ID3D12CommandQueue* commandQueue = reinterpret_cast<ID3D12CommandQueue*>(extraContext);

                    if (!device || !swapChain || !commandQueue)
                    {
                        Log::Debug("DX12 renderer init missing device, swap chain, or command queue.");
                        ImGui_ImplWin32_Shutdown();
                        return false;
                    }

                    // Create Descriptor Heap for SRV (fonts texture)
                    D3D12_DESCRIPTOR_HEAP_DESC desc = {};
                    desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
                    desc.NumDescriptors = kDx12SrvDescriptorCount;
                    desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

                    if (FAILED(device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&g_pd3d12DescriptorHeap))))
                    {
                        Log::Debug("Failed to create DX12 descriptor heap for ImGui.");
                        ImGui_ImplWin32_Shutdown();
                        return false;
                    }
                    g_dx12SrvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
                    g_dx12SrvDescriptorUsed.fill(false);

                    // Query swap chain properties to configure backbuffers
                    DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
                    if (FAILED(swapChain->GetDesc(&swapChainDesc)) || swapChainDesc.BufferCount == 0)
                    {
                        Log::Debug("Failed to query DX12 swap chain properties for ImGui.");
                        g_pd3d12DescriptorHeap->Release();
                        g_pd3d12DescriptorHeap = nullptr;
                        g_dx12SrvDescriptorSize = 0;
                        g_dx12SrvDescriptorUsed.fill(false);
                        ImGui_ImplWin32_Shutdown();
                        return false;
                    }

                    ImGui_ImplDX12_InitInfo initInfo = {};
                    initInfo.Device = device;
                    initInfo.CommandQueue = commandQueue;
                    initInfo.NumFramesInFlight = static_cast<int>(swapChainDesc.BufferCount);
                    initInfo.RTVFormat = swapChainDesc.BufferDesc.Format;
                    initInfo.SrvDescriptorHeap = g_pd3d12DescriptorHeap;
                    initInfo.SrvDescriptorAllocFn = AllocateDx12SrvDescriptor;
                    initInfo.SrvDescriptorFreeFn = FreeDx12SrvDescriptor;

                    if (!ImGui_ImplDX12_Init(&initInfo))
                    {
                        Log::Debug("ImGui_ImplDX12_Init failed.");
                        g_pd3d12DescriptorHeap->Release();
                        g_pd3d12DescriptorHeap = nullptr;
                        g_dx12SrvDescriptorSize = 0;
                        g_dx12SrvDescriptorUsed.fill(false);
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
                g_dx12SrvDescriptorSize = 0;
                g_dx12SrvDescriptorUsed.fill(false);
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
                    ID3D12GraphicsCommandList* dx12CommandList = reinterpret_cast<ID3D12GraphicsCommandList*>(commandList);
                    if (g_pd3d12DescriptorHeap)
                    {
                        ID3D12DescriptorHeap* descriptorHeaps[] = { g_pd3d12DescriptorHeap };
                        dx12CommandList->SetDescriptorHeaps(1, descriptorHeaps);
                        ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), dx12CommandList);
                    }
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
