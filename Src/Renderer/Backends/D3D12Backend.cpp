#include "Renderer/RendererBackend.h"

#include "Core/Log.h"
#include "imgui_impl_dx12.h"

#include <array>
#include <d3d12.h>
#include <dxgi1_4.h>

namespace UniversalOverlay::Renderer
{
    namespace
    {
        constexpr UINT kDx12SrvDescriptorCount = 2048;
        ID3D12DescriptorHeap* g_pd3d12DescriptorHeap = nullptr;
        UINT g_dx12SrvDescriptorSize = 0;
        std::array<bool, kDx12SrvDescriptorCount> g_dx12SrvDescriptorUsed = {};

        void ResetDx12SrvDescriptors()
        {
            if (g_pd3d12DescriptorHeap)
            {
                g_pd3d12DescriptorHeap->Release();
                g_pd3d12DescriptorHeap = nullptr;
            }

            g_dx12SrvDescriptorSize = 0;
            g_dx12SrvDescriptorUsed.fill(false);
        }

        void AllocateDx12SrvDescriptor(ImGui_ImplDX12_InitInfo*, D3D12_CPU_DESCRIPTOR_HANDLE* outCpuHandle, D3D12_GPU_DESCRIPTOR_HANDLE* outGpuHandle)
        {
            if (outCpuHandle)
                outCpuHandle->ptr = 0;
            if (outGpuHandle)
                outGpuHandle->ptr = 0;

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
        }

        void FreeDx12SrvDescriptor(ImGui_ImplDX12_InitInfo*, D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle, D3D12_GPU_DESCRIPTOR_HANDLE)
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

        bool InitializeD3D12(const RendererInitContext& context)
        {
            ID3D12Device* device = reinterpret_cast<ID3D12Device*>(context.deviceOrHdc);
            IDXGISwapChain* swapChain = reinterpret_cast<IDXGISwapChain*>(context.swapChainOrContext);
            ID3D12CommandQueue* commandQueue = reinterpret_cast<ID3D12CommandQueue*>(context.extraContext);

            if (!device || !swapChain || !commandQueue)
            {
                Log::Debug("DX12 renderer init missing device, swap chain, or command queue.");
                return false;
            }

            D3D12_DESCRIPTOR_HEAP_DESC desc = {};
            desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
            desc.NumDescriptors = kDx12SrvDescriptorCount;
            desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

            if (FAILED(device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&g_pd3d12DescriptorHeap))))
            {
                Log::Debug("Failed to create DX12 descriptor heap for ImGui.");
                return false;
            }

            g_dx12SrvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
            g_dx12SrvDescriptorUsed.fill(false);

            DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
            if (FAILED(swapChain->GetDesc(&swapChainDesc)) || swapChainDesc.BufferCount == 0)
            {
                Log::Debug("Failed to query DX12 swap chain properties for ImGui.");
                ResetDx12SrvDescriptors();
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
                ResetDx12SrvDescriptors();
                return false;
            }

            return true;
        }

        void BeginFrameD3D12()
        {
            ImGui_ImplDX12_NewFrame();
        }

        void EndFrameD3D12(const RendererFrameContext& context)
        {
            if (!context.commandList || !g_pd3d12DescriptorHeap)
                return;

            ID3D12GraphicsCommandList* dx12CommandList = reinterpret_cast<ID3D12GraphicsCommandList*>(context.commandList);
            ID3D12DescriptorHeap* descriptorHeaps[] = { g_pd3d12DescriptorHeap };
            dx12CommandList->SetDescriptorHeaps(1, descriptorHeaps);
            ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), dx12CommandList);
        }

        void ShutdownD3D12()
        {
            ImGui_ImplDX12_Shutdown();
            ResetDx12SrvDescriptors();
        }
    }

    const RendererBackendVTable& GetD3D12Backend()
    {
        static const RendererBackendVTable backend = {
            "Direct3D 12",
            GraphicsAPI::D3D12,
            InitializeD3D12,
            BeginFrameD3D12,
            EndFrameD3D12,
            ShutdownD3D12
        };
        return backend;
    }
}
