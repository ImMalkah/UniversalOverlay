#include "Hooks/Hooks.h"
#include "Core/CoreState.h"
#include "Core/Log.h"
#include "Core/OverlayLog.h"
#include "Renderer/Renderer.h"
#include "Ui/Menu.h"
#include "Ui/UIRegistry.h"
#include "Hooks/CursorHook.h"
#include "Hooks/WndProcHook.h"
#include "MinHook.h"
#include "Core/ConfigSystem.h"
#include "UniversalOverlay.h"
#include "imgui.h"
#include "imgui_impl_dx9.h"
#include "imgui_impl_dx11.h"
#include "imgui_impl_dx12.h"

#include <d3d9.h>
#include <d3d11.h>
#include <d3d12.h>
#include <dxgi1_4.h>

// Direct3D 9 Linkage
#pragma comment(lib, "d3d9.lib")
// Direct3D 11 Linkage
#pragma comment(lib, "d3d11.lib")
// Direct3D 12 Linkage
#pragma comment(lib, "d3d12.lib")
// DXGI Linkage
#pragma comment(lib, "dxgi.lib")

namespace UniversalOverlay
{
    namespace Hooks
    {
        // OpenGL Hook Definitions
        using WglSwapBuffers_t = BOOL(WINAPI*)(HDC hdc);
        static WglSwapBuffers_t oWglSwapBuffers = nullptr;

        // D3D9 Hook Definitions
        using EndSceneD3D9_t = HRESULT(APIENTRY*)(IDirect3DDevice9* device);
        using ResetD3D9_t = HRESULT(APIENTRY*)(IDirect3DDevice9* device, D3DPRESENT_PARAMETERS* params);
        static EndSceneD3D9_t oEndSceneD3D9 = nullptr;
        static ResetD3D9_t oResetD3D9 = nullptr;

        // D3D11 / D3D12 Hook Definitions
        using PresentDXGI_t = HRESULT(APIENTRY*)(IDXGISwapChain* swapChain, UINT syncInterval, UINT flags);
        using ResizeBuffersDXGI_t = HRESULT(APIENTRY*)(IDXGISwapChain* swapChain, UINT bufferCount, UINT width, UINT height, DXGI_FORMAT format, UINT flags);
        static PresentDXGI_t oPresentD3D11 = nullptr;
        static ResizeBuffersDXGI_t oResizeBuffersD3D11 = nullptr;

        static PresentDXGI_t oPresentD3D12 = nullptr;
        static ResizeBuffersDXGI_t oResizeBuffersD3D12 = nullptr;

        using ExecuteCommandLists_t = void(APIENTRY*)(ID3D12CommandQueue* queue, UINT numCommandLists, ID3D12CommandList* const* commandLists);
        static ExecuteCommandLists_t oExecuteCommandLists = nullptr;

        // Global Graphics States
        static ID3D11RenderTargetView* g_d3d11RenderTargetView = nullptr;
        static ID3D12CommandQueue* g_d3d12CommandQueue = nullptr;
        static std::vector<ID3D12Resource*> g_d3d12BackBuffers;
        static std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> g_d3d12RtvHandles;
        static ID3D12DescriptorHeap* g_d3d12RtvDescriptorHeap = nullptr;
        static ID3D12GraphicsCommandList* g_d3d12CommandList = nullptr;
        static ID3D12CommandAllocator* g_d3d12CommandAllocator = nullptr;

        static void WaitForD3D12QueueIdle()
        {
            if (!g_d3d12CommandQueue)
                return;

            ID3D12Device* device = nullptr;
            if (FAILED(g_d3d12CommandQueue->GetDevice(IID_PPV_ARGS(&device))) || !device)
                return;

            ID3D12Fence* fence = nullptr;
            if (FAILED(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence))) || !fence)
            {
                device->Release();
                return;
            }

            HANDLE eventHandle = CreateEventW(nullptr, FALSE, FALSE, nullptr);
            if (!eventHandle)
            {
                fence->Release();
                device->Release();
                return;
            }

            constexpr UINT64 fenceValue = 1;
            if (SUCCEEDED(g_d3d12CommandQueue->Signal(fence, fenceValue)))
            {
                if (fence->GetCompletedValue() < fenceValue &&
                    SUCCEEDED(fence->SetEventOnCompletion(fenceValue, eventHandle)))
                {
                    const DWORD waitResult = WaitForSingleObject(eventHandle, 2000);
                    if (waitResult != WAIT_OBJECT_0)
                        Log::Debug("Timed out waiting for DX12 queue idle during shutdown.");
                }
            }

            CloseHandle(eventHandle);
            fence->Release();
            device->Release();
        }

        struct HookRefGuard
        {
            HookRefGuard() { State::hookRefCount++; }
            ~HookRefGuard() { State::hookRefCount--; }
        };

        // Menu Sync Logic
        static void ApplyMenuState()
        {
            SetMenuOpenForWndProc(State::menuOpen);
            Renderer::SetDrawCursor(State::menuOpen);
            ApplyCursorState(State::windowHandle, State::menuOpen);
        }

        static void SyncMenuState()
        {
            static bool previousMenuOpen = State::menuOpen;
            if (previousMenuOpen != State::menuOpen)
            {
                ApplyMenuState();
                ConfigSystem::MarkDirty();
                if (!State::menuOpen)
                {
                    ConfigSystem::SaveIfDirty(ConfigSystem::GetConfigPath());
                }
                previousMenuOpen = State::menuOpen;
            }
        }

        void HandleMenuHotkey()
        {
            if (State::waitingForMenuToggleKey || State::waitingForUnloadKey)
                return;

            // Toggle menu hotkey
            static bool keyWasDown = false;
            bool keyDown = IsHotkeyComboPressed(State::menuToggleKey);

            if (keyDown && !keyWasDown)
            {
                State::menuOpen = !State::menuOpen;
                ConfigSystem::MarkDirty();
                Log::Debug("Menu hotkey toggled menuOpen=%s", State::menuOpen ? "true" : "false");
            }
            keyWasDown = keyDown;

            // Check unload hotkey
            static bool unloadWasDown = false;
            bool unloadDown = IsHotkeyComboPressed(State::unloadKey);

            if (unloadDown && !unloadWasDown)
            {
                Log::Debug("Unload hotkey detected!");
                RequestUnload();
            }
            unloadWasDown = unloadDown;
        }

        static bool BeginOverlayFrameIfActive(HWND hwnd)
        {
            HandleMenuHotkey();
            SyncMenuState();

            if (State::shouldUnload.load())
            {
                State::menuOpen = false;
                ApplyMenuState();
                ApplyCursorState(hwnd, false);
                return false;
            }

            ApplyCursorState(hwnd, State::menuOpen);
            return true;
        }

        static void DrawOverlayFrame(void* commandList = nullptr)
        {
            Renderer::BeginFrame();
            if (!State::shouldUnload.load() && State::renderCallback)
            {
                State::renderCallback();
            }
            if (!State::shouldUnload.load() && State::menuOpen)
            {
                Menu::Draw();
            }
            if (!State::shouldUnload.load())
            {
                UIRegistry::DrawFloatingWindows(State::menuOpen);
            }
            Renderer::EndFrame(commandList);
        }

        // OpenGL SwapBuffers Hook
        BOOL WINAPI hkWglSwapBuffers(HDC hdc)
        {
            HookRefGuard guard;
            HWND hwnd = WindowFromDC(hdc);
            if (!hwnd)
                return oWglSwapBuffers(hdc);

            if (!Renderer::IsInitialized())
            {
                State::windowHandle = hwnd;
                Renderer::Initialize(hwnd, GraphicsAPI::OpenGL3, hdc);
                InstallWndProcHook(hwnd);
                ApplyMenuState();
            }

            if (!BeginOverlayFrameIfActive(hwnd))
                return oWglSwapBuffers(hdc);

            DrawOverlayFrame();

            SyncMenuState();

            return oWglSwapBuffers(hdc);
        }

        // D3D9 EndScene & Reset Hooks
        HRESULT APIENTRY hkEndSceneD3D9(IDirect3DDevice9* device)
        {
            HookRefGuard guard;
            if (!Renderer::IsInitialized())
            {
                D3DDEVICE_CREATION_PARAMETERS params;
                device->GetCreationParameters(&params);
                HWND hwnd = params.hFocusWindow;
                if (!hwnd) hwnd = GetActiveWindow();

                State::windowHandle = hwnd;
                Renderer::Initialize(hwnd, GraphicsAPI::D3D9, device);
                InstallWndProcHook(hwnd);
                ApplyMenuState();
            }

            if (!BeginOverlayFrameIfActive(State::windowHandle))
                return oEndSceneD3D9(device);

            DrawOverlayFrame();

            SyncMenuState();

            return oEndSceneD3D9(device);
        }

        HRESULT APIENTRY hkResetD3D9(IDirect3DDevice9* device, D3DPRESENT_PARAMETERS* params)
        {
            HookRefGuard guard;
            ImGui_ImplDX9_InvalidateDeviceObjects();
            HRESULT result = oResetD3D9(device, params);
            ImGui_ImplDX9_CreateDeviceObjects();
            return result;
        }

        // D3D11 Present & ResizeBuffers Hooks
        HRESULT APIENTRY hkPresentD3D11(IDXGISwapChain* swapChain, UINT syncInterval, UINT flags)
        {
            HookRefGuard guard;
            if (!swapChain || !oPresentD3D11)
                return DXGI_ERROR_INVALID_CALL;

            if (!Renderer::IsInitialized())
            {
                DXGI_SWAP_CHAIN_DESC desc = {};
                if (FAILED(swapChain->GetDesc(&desc)))
                    return oPresentD3D11(swapChain, syncInterval, flags);

                HWND hwnd = desc.OutputWindow;

                ID3D11Device* device = nullptr;
                HRESULT hr = swapChain->GetDevice(__uuidof(ID3D11Device), reinterpret_cast<void**>(&device));
                if (FAILED(hr) || !device)
                {
                    static bool loggedDeviceFailure = false;
                    if (!loggedDeviceFailure)
                    {
                        Log::Debug("D3D11 Present skipped: swap chain does not expose ID3D11Device. HRESULT=0x%08X", static_cast<unsigned int>(hr));
                        loggedDeviceFailure = true;
                    }
                    return oPresentD3D11(swapChain, syncInterval, flags);
                }

                ID3D11DeviceContext* context = nullptr;
                device->GetImmediateContext(&context);
                if (!context)
                {
                    device->Release();
                    return oPresentD3D11(swapChain, syncInterval, flags);
                }

                State::windowHandle = hwnd;
                if (!Renderer::Initialize(hwnd, GraphicsAPI::D3D11, device, context))
                {
                    context->Release();
                    device->Release();
                    return oPresentD3D11(swapChain, syncInterval, flags);
                }

                if (hwnd)
                {
                    InstallWndProcHook(hwnd);
                    ApplyMenuState();
                }

                device->Release();
                context->Release();
            }

            if (!g_d3d11RenderTargetView)
            {
                ID3D11Device* device = nullptr;
                if (FAILED(swapChain->GetDevice(__uuidof(ID3D11Device), reinterpret_cast<void**>(&device))) || !device)
                    return oPresentD3D11(swapChain, syncInterval, flags);

                ID3D11Texture2D* backBuffer = nullptr;
                HRESULT hr = swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&backBuffer));
                if (FAILED(hr) || !backBuffer)
                {
                    device->Release();
                    return oPresentD3D11(swapChain, syncInterval, flags);
                }

                hr = device->CreateRenderTargetView(backBuffer, nullptr, &g_d3d11RenderTargetView);
                
                backBuffer->Release();
                device->Release();

                if (FAILED(hr) || !g_d3d11RenderTargetView)
                    return oPresentD3D11(swapChain, syncInterval, flags);
            }

            if (!BeginOverlayFrameIfActive(State::windowHandle))
                return oPresentD3D11(swapChain, syncInterval, flags);

            if (g_d3d11RenderTargetView)
            {
                ID3D11Device* device = nullptr;
                if (FAILED(swapChain->GetDevice(__uuidof(ID3D11Device), reinterpret_cast<void**>(&device))) || !device)
                    return oPresentD3D11(swapChain, syncInterval, flags);

                ID3D11DeviceContext* context = nullptr;
                device->GetImmediateContext(&context);
                if (!context)
                {
                    device->Release();
                    return oPresentD3D11(swapChain, syncInterval, flags);
                }

                context->OMSetRenderTargets(1, &g_d3d11RenderTargetView, nullptr);
                DrawOverlayFrame();

                context->Release();
                device->Release();
            }

            SyncMenuState();

            return oPresentD3D11(swapChain, syncInterval, flags);
        }

        HRESULT APIENTRY hkResizeBuffersD3D11(IDXGISwapChain* swapChain, UINT bufferCount, UINT width, UINT height, DXGI_FORMAT format, UINT flags)
        {
            HookRefGuard guard;
            if (g_d3d11RenderTargetView)
            {
                g_d3d11RenderTargetView->Release();
                g_d3d11RenderTargetView = nullptr;
            }
            return oResizeBuffersD3D11(swapChain, bufferCount, width, height, format, flags);
        }

        // D3D12 ExecuteCommandLists & Present & ResizeBuffers Hooks
        void APIENTRY hkExecuteCommandLists(ID3D12CommandQueue* queue, UINT numCommandLists, ID3D12CommandList* const* commandLists)
        {
            HookRefGuard guard;
            if (!g_d3d12CommandQueue && queue->GetDesc().Type == D3D12_COMMAND_LIST_TYPE_DIRECT)
            {
                g_d3d12CommandQueue = queue;
                Log::Debug("DirectX 12 Direct Command Queue intercepted: 0x%p", g_d3d12CommandQueue);
            }
            oExecuteCommandLists(queue, numCommandLists, commandLists);
        }

        HRESULT APIENTRY hkPresentD3D12(IDXGISwapChain* swapChain, UINT syncInterval, UINT flags)
        {
            HookRefGuard guard;
            if (!swapChain || !oPresentD3D12)
                return DXGI_ERROR_INVALID_CALL;

            if (!g_d3d12CommandQueue)
                return oPresentD3D12(swapChain, syncInterval, flags);

            ID3D12Device* device = nullptr;
            HRESULT hr = swapChain->GetDevice(__uuidof(ID3D12Device), reinterpret_cast<void**>(&device));
            if (FAILED(hr) || !device)
                return oPresentD3D12(swapChain, syncInterval, flags);

            if (!Renderer::IsInitialized())
            {
                DXGI_SWAP_CHAIN_DESC desc = {};
                if (FAILED(swapChain->GetDesc(&desc)) || desc.BufferCount == 0)
                {
                    device->Release();
                    return oPresentD3D12(swapChain, syncInterval, flags);
                }

                HWND hwnd = desc.OutputWindow;

                State::windowHandle = hwnd;
                if (!Renderer::Initialize(hwnd, GraphicsAPI::D3D12, device, swapChain, g_d3d12CommandQueue))
                {
                    device->Release();
                    return oPresentD3D12(swapChain, syncInterval, flags);
                }

                if (hwnd)
                {
                    InstallWndProcHook(hwnd);
                    ApplyMenuState();
                }

                // Allocate RTV Descriptor Heap
                D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
                rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
                rtvHeapDesc.NumDescriptors = desc.BufferCount;
                rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
                if (FAILED(device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&g_d3d12RtvDescriptorHeap))) || !g_d3d12RtvDescriptorHeap)
                {
                    Renderer::Shutdown();
                    device->Release();
                    return oPresentD3D12(swapChain, syncInterval, flags);
                }

                UINT rtvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
                D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = g_d3d12RtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();

                g_d3d12BackBuffers.resize(desc.BufferCount);
                g_d3d12RtvHandles.resize(desc.BufferCount);

                for (UINT i = 0; i < desc.BufferCount; i++)
                {
                    g_d3d12RtvHandles[i] = rtvHandle;
                    if (FAILED(swapChain->GetBuffer(i, IID_PPV_ARGS(&g_d3d12BackBuffers[i]))) || !g_d3d12BackBuffers[i])
                    {
                        Renderer::Shutdown();
                        device->Release();
                        return oPresentD3D12(swapChain, syncInterval, flags);
                    }
                    device->CreateRenderTargetView(g_d3d12BackBuffers[i], nullptr, rtvHandle);
                    rtvHandle.ptr += rtvDescriptorSize;
                }

                // Command allocator and list
                if (FAILED(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&g_d3d12CommandAllocator))) || !g_d3d12CommandAllocator)
                {
                    Renderer::Shutdown();
                    device->Release();
                    return oPresentD3D12(swapChain, syncInterval, flags);
                }

                if (FAILED(device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, g_d3d12CommandAllocator, nullptr, IID_PPV_ARGS(&g_d3d12CommandList))) || !g_d3d12CommandList)
                {
                    Renderer::Shutdown();
                    device->Release();
                    return oPresentD3D12(swapChain, syncInterval, flags);
                }
                g_d3d12CommandList->Close();
            }

            if (!g_d3d12CommandAllocator || !g_d3d12CommandList || g_d3d12BackBuffers.empty() || g_d3d12RtvHandles.empty())
            {
                device->Release();
                return oPresentD3D12(swapChain, syncInterval, flags);
            }

            if (!BeginOverlayFrameIfActive(State::windowHandle))
            {
                device->Release();
                return oPresentD3D12(swapChain, syncInterval, flags);
            }

            // Execute frame rendering commands
            if (FAILED(g_d3d12CommandAllocator->Reset()) || FAILED(g_d3d12CommandList->Reset(g_d3d12CommandAllocator, nullptr)))
            {
                device->Release();
                return oPresentD3D12(swapChain, syncInterval, flags);
            }

            IDXGISwapChain3* swapChain3 = nullptr;
            if (FAILED(swapChain->QueryInterface(IID_PPV_ARGS(&swapChain3))) || !swapChain3)
            {
                device->Release();
                return oPresentD3D12(swapChain, syncInterval, flags);
            }

            UINT backBufferIndex = swapChain3->GetCurrentBackBufferIndex();
            swapChain3->Release();

            if (backBufferIndex >= g_d3d12BackBuffers.size() || backBufferIndex >= g_d3d12RtvHandles.size() || !g_d3d12BackBuffers[backBufferIndex])
            {
                device->Release();
                return oPresentD3D12(swapChain, syncInterval, flags);
            }

            // Transition buffer from Present to RenderTarget
            D3D12_RESOURCE_BARRIER barrier = {};
            barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
            barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
            barrier.Transition.pResource = g_d3d12BackBuffers[backBufferIndex];
            barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
            barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
            barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
            g_d3d12CommandList->ResourceBarrier(1, &barrier);

            g_d3d12CommandList->OMSetRenderTargets(1, &g_d3d12RtvHandles[backBufferIndex], FALSE, nullptr);

            DrawOverlayFrame(g_d3d12CommandList);

            // Transition buffer back to Present
            barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
            barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
            g_d3d12CommandList->ResourceBarrier(1, &barrier);

            g_d3d12CommandList->Close();
            ID3D12CommandList* ppCommandLists[] = { g_d3d12CommandList };
            g_d3d12CommandQueue->ExecuteCommandLists(1, ppCommandLists);

            device->Release();
            SyncMenuState();

            return oPresentD3D12(swapChain, syncInterval, flags);
        }

        HRESULT APIENTRY hkResizeBuffersD3D12(IDXGISwapChain* swapChain, UINT bufferCount, UINT width, UINT height, DXGI_FORMAT format, UINT flags)
        {
            HookRefGuard guard;
            // Release back buffers references
            for (auto& buffer : g_d3d12BackBuffers)
            {
                if (buffer)
                {
                    buffer->Release();
                    buffer = nullptr;
                }
            }
            g_d3d12BackBuffers.clear();
            g_d3d12RtvHandles.clear();

            if (g_d3d12RtvDescriptorHeap)
            {
                g_d3d12RtvDescriptorHeap->Release();
                g_d3d12RtvDescriptorHeap = nullptr;
            }

            if (g_d3d12CommandAllocator)
            {
                g_d3d12CommandAllocator->Release();
                g_d3d12CommandAllocator = nullptr;
            }

            if (g_d3d12CommandList)
            {
                g_d3d12CommandList->Release();
                g_d3d12CommandList = nullptr;
            }

            Renderer::Shutdown(); // Forces reinitialization on next Present

            return oResizeBuffersD3D12(swapChain, bufferCount, width, height, format, flags);
        }

        // Install API-specific hooks
        bool Install(GraphicsAPI api)
        {
            State::api = api;

            if (MH_Initialize() != MH_OK)
            {
                Log::Debug("MinHook initialization failed.");
                return false;
            }

            // Create temporary dummy window
            HWND dummyWnd = CreateWindowA("BUTTON", "DummyOverlayWnd", WS_SYSMENU | WS_MINIMIZEBOX, 100, 100, 100, 100, NULL, NULL, GetModuleHandle(NULL), NULL);

            if (api == GraphicsAPI::OpenGL3)
            {
                HMODULE openGL = GetModuleHandleA("opengl32.dll");
                if (!openGL) return false;
                void* target = GetProcAddress(openGL, "wglSwapBuffers");
                
                MH_CreateHook(target, &hkWglSwapBuffers, reinterpret_cast<void**>(&oWglSwapBuffers));
            }
            else if (api == GraphicsAPI::D3D9)
            {
                IDirect3D9* d3d = Direct3DCreate9(D3D_SDK_VERSION);
                if (!d3d) return false;

                D3DPRESENT_PARAMETERS params = {};
                params.Windowed = TRUE;
                params.SwapEffect = D3DSWAPEFFECT_DISCARD;
                params.hDeviceWindow = dummyWnd;

                IDirect3DDevice9* device = nullptr;
                if (SUCCEEDED(d3d->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, dummyWnd, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &params, &device)))
                {
                    void** vtable = *reinterpret_cast<void***>(device);
                    MH_CreateHook(vtable[42], &hkEndSceneD3D9, reinterpret_cast<void**>(&oEndSceneD3D9));
                    MH_CreateHook(vtable[16], &hkResetD3D9, reinterpret_cast<void**>(&oResetD3D9));
                    device->Release();
                }
                d3d->Release();
            }
            else if (api == GraphicsAPI::D3D11)
            {
                D3D_FEATURE_LEVEL featureLevel;
                IDXGISwapChain* swapChain = nullptr;
                ID3D11Device* device = nullptr;
                ID3D11DeviceContext* context = nullptr;

                DXGI_SWAP_CHAIN_DESC scDesc = {};
                scDesc.BufferCount = 1;
                scDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
                scDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
                scDesc.OutputWindow = dummyWnd;
                scDesc.SampleDesc.Count = 1;
                scDesc.Windowed = TRUE;

                if (SUCCEEDED(D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_NULL, nullptr, 0, nullptr, 0, D3D11_SDK_VERSION, &scDesc, &swapChain, &device, &featureLevel, &context)))
                {
                    void** vtable = *reinterpret_cast<void***>(swapChain);
                    MH_CreateHook(vtable[8], &hkPresentD3D11, reinterpret_cast<void**>(&oPresentD3D11));
                    MH_CreateHook(vtable[13], &hkResizeBuffersD3D11, reinterpret_cast<void**>(&oResizeBuffersD3D11));
                    
                    swapChain->Release();
                    device->Release();
                    context->Release();
                }
            }
            else if (api == GraphicsAPI::D3D12)
            {
                ID3D12Device* device = nullptr;
                if (SUCCEEDED(D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&device))))
                {
                    IDXGIFactory4* factory = nullptr;
                    CreateDXGIFactory1(IID_PPV_ARGS(&factory));

                    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
                    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
                    ID3D12CommandQueue* queue = nullptr;
                    device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&queue));

                    DXGI_SWAP_CHAIN_DESC scDesc = {};
                    scDesc.BufferCount = 2;
                    scDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
                    scDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
                    scDesc.OutputWindow = dummyWnd;
                    scDesc.SampleDesc.Count = 1;
                    scDesc.Windowed = TRUE;
                    scDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

                    IDXGISwapChain* swapChain = nullptr;
                    if (factory && queue && SUCCEEDED(factory->CreateSwapChain(queue, &scDesc, &swapChain)))
                    {
                        void** vtable = *reinterpret_cast<void***>(swapChain);
                        MH_CreateHook(vtable[8], &hkPresentD3D12, reinterpret_cast<void**>(&oPresentD3D12));
                        MH_CreateHook(vtable[13], &hkResizeBuffersD3D12, reinterpret_cast<void**>(&oResizeBuffersD3D12));

                        void** queueVtable = *reinterpret_cast<void***>(queue);
                        MH_CreateHook(queueVtable[10], &hkExecuteCommandLists, reinterpret_cast<void**>(&oExecuteCommandLists));

                        swapChain->Release();
                    }

                    if (queue) queue->Release();
                    if (factory) factory->Release();
                    device->Release();
                }
            }

            DestroyWindow(dummyWnd);

            if (!InstallCursorHooks())
            {
                Log::Debug("Cursor hooks failed to install.");
                return false;
            }

            if (MH_EnableHook(MH_ALL_HOOKS) != MH_OK)
            {
                Log::Debug("Failed to enable MinHook hooks.");
                return false;
            }

            Log::Debug("Hooks successfully configured and armed.");
            return true;
        }

        // Uninstall hooks
        void Remove()
        {
            Log::Debug("Disabling active hooks...");
            MH_DisableHook(MH_ALL_HOOKS);
            
            // Wait for all threads to exit the hooks detours safely
            while (State::hookRefCount > 0)
            {
                Sleep(1);
            }

            MH_Uninitialize();
            
            RemoveCursorHooks();
            RemoveWndProcHook();

            WaitForD3D12QueueIdle();

            // Clean up graphics objects
            if (g_d3d11RenderTargetView)
            {
                g_d3d11RenderTargetView->Release();
                g_d3d11RenderTargetView = nullptr;
            }

            for (auto& buffer : g_d3d12BackBuffers)
            {
                if (buffer) buffer->Release();
            }
            g_d3d12BackBuffers.clear();

            if (g_d3d12RtvDescriptorHeap)
            {
                g_d3d12RtvDescriptorHeap->Release();
                g_d3d12RtvDescriptorHeap = nullptr;
            }

            if (g_d3d12CommandAllocator)
            {
                g_d3d12CommandAllocator->Release();
                g_d3d12CommandAllocator = nullptr;
            }

            if (g_d3d12CommandList)
            {
                g_d3d12CommandList->Release();
                g_d3d12CommandList = nullptr;
            }

            g_d3d12CommandQueue = nullptr;
        }
    }
}
