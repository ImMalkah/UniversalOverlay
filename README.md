# UniversalOverlay

UniversalOverlay is a C++23 static library for injected Win32 overlays. It uses Dear ImGui and MinHook to hook a game's render path, draw a menu, route input, persist settings, and unload cleanly.

Use it when you want to build a debugging menu, cheat menu, SDK inspection panel, or always-on overlay without writing the renderer hooks and ImGui plumbing every time.

## What It Gives You

- **Renderer hooks:** OpenGL 3, Direct3D 9, Direct3D 11, and Direct3D 12.
- **Main menu shell:** register your own tabs, then use the built-in `Settings`, `UI Gallery`, and `SDK Debug` tabs for common overlay work.
- **Settings and presets:** register `bool`, `float`, and `int` values, load/save an INI file, and use five named preset slots.
- **Per-frame drawing:** register one render callback for ESP text, debug labels, meters, boxes, crosshairs, or other drawlist work.
- **Managed floating windows:** create windows for things like entity inspectors, memory watches, player lists, or live logs. Open/pin/position/size state is persisted.
- **Input and cursor handling:** when the menu is open, the hooked `WndProc` feeds ImGui and cursor hooks release the game's cursor lock. Closing the menu restores normal input behavior.
- **UI helpers:** theme tokens, compact layouts, status pills, badges, meters, gradient/draw helpers, font loading, icon labels, and a UI gallery.
- **Diagnostics:** rotating logs under `%TEMP%\UniversalOverlay\<process-id>\` through `External/spdlog`.
- **Safe unload path:** `ShouldUnload()` and `RequestUnload()` let your host thread shut down hooks, renderer state, ImGui, and logging in order.

## Overlay Semantics

- `Initialize(api)` installs hooks for one graphics API. The renderer itself initializes lazily on the first real frame/swap call from the host process.
- Per frame, UniversalOverlay handles hotkeys, syncs cursor/input state, starts an ImGui frame, runs your render callback, draws the main menu if open, draws managed floating windows, then submits ImGui draw data.
- `RegisterRenderCallback()` is for always-on drawlist work. It runs every frame while the overlay is active, even when the menu is closed.
- `RegisterTab()` adds a tab to the main menu. Your tab callback runs only while the menu is open and that tab is selected.
- `RegisterSettingsSection()` adds controls to the built-in `Settings` tab. Register those values with `RegisterConfigBool/Float/Int` so saves and presets include them.
- `RegisterFloatingWindow()` creates a named ImGui window. Unpinned windows show only while the menu is open. Pinned windows stay visible when the menu closes, but they stop accepting input until the menu opens again.
- Register config values before `LoadConfig()`. Call `MarkConfigDirty()` when an ImGui control changes a registered value.
- Default hotkeys are `.` for menu toggle and `F1` for unload. They can be rebound in the `Settings` tab.

## Rendering Modes

Pick the mode that matches the target process. Switching modes is just changing the argument passed to `Initialize()` and rebuilding/reloading your host DLL.

```cpp
UniversalOverlay::Initialize(UniversalOverlay::GraphicsAPI::D3D11);
```

| Mode | Enum | Hook path | Notes |
| --- | --- | --- | --- |
| OpenGL 3 | `GraphicsAPI::OpenGL3` | `wglSwapBuffers` | For Win32 OpenGL targets using the OpenGL3 ImGui backend. |
| Direct3D 9 | `GraphicsAPI::D3D9` | `IDirect3DDevice9::EndScene` and `Reset` | Handles device reset by invalidating/recreating ImGui DX9 objects. |
| Direct3D 11 | `GraphicsAPI::D3D11` | `IDXGISwapChain::Present` and `ResizeBuffers` | Good default for many DXGI games/tools. |
| Direct3D 12 | `GraphicsAPI::D3D12` | `Present`, `ResizeBuffers`, and command queue `ExecuteCommandLists` | Captures the direct command queue before rendering ImGui. |

Only initialize one mode per overlay lifetime. If you need runtime selection, choose the enum before `Initialize()`:

```cpp
UniversalOverlay::GraphicsAPI api = UniversalOverlay::GraphicsAPI::D3D11;

// Example: change this from your loader/config before calling Initialize.
if (useDx9)
    api = UniversalOverlay::GraphicsAPI::D3D9;
else if (useOpenGl)
    api = UniversalOverlay::GraphicsAPI::OpenGL3;

if (!UniversalOverlay::Initialize(api))
    return;
```

## Getting Started

### 1. Build The Library

Open `UniversalOverlay.sln` in Visual Studio 2022, select `Debug` or `Release`, select `x86` or `x64`, then build. Output goes to:

```text
Build/[Configuration]/[Platform]/UniversalOverlay.lib
```

### 2. Link Your DLL

In your injected DLL project:

- Add `Src/`, `External/ImGui`, `External/ImGui/Backends`, and `External/MinHook/include` to include directories.
- Link `UniversalOverlay.lib` and the matching `libMinHook.x86.lib` or `libMinHook.x64.lib`.
- Use C++23 (`/std:c++23`).
- Include `UniversalOverlay.h` and `imgui.h`.

### 3. Basic Example

Copy this into a DLL project, link the library, and change the `GraphicsAPI` enum for your target renderer.

```cpp
#include <Windows.h>
#include <cstdio>
#include <filesystem>
#include <string>

#include "UniversalOverlay.h"
#include "imgui.h"

namespace
{
    bool g_showOverlayText = true;
    float g_overlayAlpha = 0.85f;
    int g_statusMode = 1;

    std::wstring GetConfigPath(HMODULE module)
    {
        wchar_t modulePath[MAX_PATH] = {};
        const DWORD length = GetModuleFileNameW(module, modulePath, MAX_PATH);

        if (length == 0)
            return L".\\UniversalOverlaySettings.ini";

        return (std::filesystem::path(modulePath).parent_path() / L"UniversalOverlaySettings.ini").wstring();
    }

    const char* GetStatusText()
    {
        switch (g_statusMode)
        {
        case 0: return "Idle";
        case 2: return "Warning";
        default: return "Tracking";
        }
    }

    void DrawExampleSettings()
    {
        bool changed = false;
        changed |= ImGui::Checkbox("Show overlay text", &g_showOverlayText);
        changed |= ImGui::SliderFloat("Overlay alpha", &g_overlayAlpha, 0.10f, 1.0f, "%.2f");
        changed |= ImGui::Combo("Status mode", &g_statusMode, "Idle\0Tracking\0Warning\0\0");

        if (changed)
            UniversalOverlay::MarkConfigDirty();
    }

    void DrawExampleTab()
    {
        ImGui::TextUnformatted("Debug menu starter");
        ImGui::Separator();
        DrawExampleSettings();

        if (ImGui::Button("Open status window"))
            UniversalOverlay::SetFloatingWindowOpen("Example Status", true);

        ImGui::SameLine();
        if (ImGui::Button("Save preset 1"))
            UniversalOverlay::SaveConfigPreset(1);

        ImGui::SameLine();
        if (ImGui::Button("Load preset 1"))
            UniversalOverlay::LoadConfigPreset(1);
    }

    void DrawExampleStatusWindow(bool menuOpen)
    {
        ImGui::Text("Status: %s", GetStatusText());

        if (menuOpen)
        {
            ImGui::Separator();
            DrawExampleSettings();
        }
    }

    void DrawExampleOverlay()
    {
        if (!g_showOverlayText)
            return;

        char label[96] = {};
        sprintf_s(label, "UniversalOverlay: %s", GetStatusText());

        const int alpha = static_cast<int>(255.0f * g_overlayAlpha);
        ImGui::GetBackgroundDrawList()->AddText(
            ImVec2(20.0f, 20.0f),
            IM_COL32(80, 220, 255, alpha),
            label);
    }

    DWORD WINAPI OverlayThread(LPVOID parameter)
    {
        HMODULE module = static_cast<HMODULE>(parameter);
        const std::wstring configPath = GetConfigPath(module);

        if (!UniversalOverlay::Initialize(UniversalOverlay::GraphicsAPI::D3D11))
            FreeLibraryAndExitThread(module, 0);

        UniversalOverlay::SetMenuDefaultSize(980.0f, 640.0f);

        UniversalOverlay::RegisterConfigBool("Example", "ShowOverlayText", &g_showOverlayText);
        UniversalOverlay::RegisterConfigFloat("Example", "OverlayAlpha", &g_overlayAlpha, 0.85f);
        UniversalOverlay::RegisterConfigInt("Example", "StatusMode", &g_statusMode, 1);
        UniversalOverlay::LoadConfig(configPath);

        UniversalOverlay::RegisterSettingsSection("Example", DrawExampleSettings);
        UniversalOverlay::RegisterTab("Example", DrawExampleTab);
        UniversalOverlay::RegisterFloatingWindow("Example Status", DrawExampleStatusWindow, true, false, 0.45f);
        UniversalOverlay::RegisterRenderCallback(DrawExampleOverlay);

        while (!UniversalOverlay::ShouldUnload())
            Sleep(100);

        UniversalOverlay::SaveConfig(configPath);
        UniversalOverlay::Shutdown();
        FreeLibraryAndExitThread(module, 0);
        return 0;
    }
}

BOOL APIENTRY DllMain(HMODULE module, DWORD reason, LPVOID)
{
    if (reason != DLL_PROCESS_ATTACH)
        return TRUE;

    DisableThreadLibraryCalls(module);

    HANDLE thread = CreateThread(nullptr, 0, OverlayThread, module, 0, nullptr);
    if (thread)
        CloseHandle(thread);

    return TRUE;
}
```

## API You Will Use Most

- `Initialize(GraphicsAPI api)` / `Shutdown()`
- `ShouldUnload()` / `RequestUnload()`
- `IsMenuOpen()` / `SetMenuOpen(bool open)`
- `SetMenuDefaultSize(float width, float height)`
- `RegisterTab(name, callback)`
- `RegisterSettingsSection(name, callback)`
- `RegisterRenderCallback(callback)`
- `RegisterFloatingWindow(name, callback, defaultOpen, defaultPinned, backgroundAlpha)`
- `SetFloatingWindowOpen(name, open)` / `SetFloatingWindowPinned(name, pinned)`
- `RegisterConfigBool/Float/Int(section, key, pointer, defaultValue)`
- `LoadConfig(path)` / `SaveConfig(path)`
- `SaveConfigPreset(slot)` / `LoadConfigPreset(slot)`

## Source Map

Most users only need `Src/UniversalOverlay.h`. Useful internals:

- `Src/Core/UniversalOverlayCore.cpp`, `ConfigSystem.h/cpp`, `OverlayLog.h/cpp`; logging is backed by `External/spdlog`.
- `Src/Hooks/Hooks.h/cpp`, `WndProcHook.h/cpp`, and `CursorHook.h/cpp`.
- `Src/Renderer/Renderer.h/cpp`, `RendererTypes.h`, and `Renderer/Backends`.
- UI shell/helpers: `Menu.h/cpp`, `UIRegistry.h/cpp`, `OverlayTheme.h/cpp`, `OverlayLayout.h/cpp`, `OverlayDraw.h/cpp`, `OverlayWidgets.h/cpp`, `OverlayWindowManager.h/cpp`, `OverlayFonts.h/cpp`, `OverlayIcons.h/cpp`, `OverlayGallery.h/cpp`.

Extra notes live in `docs/wiki/`, especially logging/diagnostics and third-party dependency notes.

## License

MIT. See `LICENSE`.
