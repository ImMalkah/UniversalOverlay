# UniversalOverlay

[![C++ Standard](https://img.shields.io/badge/C%2B%2B-23-blue.svg?style=flat-square&logo=cplusplus)](https://en.cppreference.com/w/cpp/compiler_support/23)
[![Platform](https://img.shields.io/badge/Platform-Windows-lightgrey.svg?style=flat-square&logo=windows)](https://docs.microsoft.com/en-us/windows/)
[![Graphics APIs](https://img.shields.io/badge/APIs-DX9%20%7C%20DX10%20guard%20%7C%20DX11%20%7C%20DX12%20%7C%20OpenGL3-orange.svg?style=flat-square)](https://github.com/ocornut/imgui)
[![License](https://img.shields.io/badge/License-MIT-green.svg?style=flat-square)](https://opensource.org/licenses/MIT)

**UniversalOverlay** is a lightweight, modern, and highly modular C++23 static library designed to hook into target processes (games or 3D applications) and inject a custom overlay menu and rendering callbacks.

Powered by **Dear ImGui**, **MinHook**, and **spdlog**, it intercepts render loops, hijacks window input (`WndProc`), manages cursor locking state, and provides a built-in INI configuration system with keybinding collision resolution.

---

## 🚀 Key Features

*   **Multi-API Support**: Out-of-the-box detours for:
    *   **OpenGL 3** (detouring `wglSwapBuffers`)
    *   **Direct3D 9** (detouring vtable `EndScene` & `Reset`)
    *   **Direct3D 10** (registered as a guarded API target; runtime hook support is intentionally disabled for now)
    *   **Direct3D 11** (detouring vtable `Present` & `ResizeBuffers`)
    *   **Direct3D 12** (detouring vtable `Present`, `ResizeBuffers`, & command queue `ExecuteCommandLists`)
*   **Dynamic VTable Resolution**: Avoids hardcoded virtual table offsets by instantiating temporary dummy windows and graphics devices at startup to resolve runtime function addresses.
*   **Seamless Input Hijacking**: Hooked `WndProc` redirects keyboard and mouse inputs to ImGui. Cursor hooks (`SetCursor`, `ClipCursor`, `ShowCursor`) are automatically detoured to release/re-lock the cursor when the menu state changes.
*   **Built-in Configuration System**: Easily register `bool`, `float`, and `int` settings. Features automated disk serialization/deserialization to INI files, five named preset slots, interactive menu keybind capturing, built-in theme persistence, and menu/floating-window state persistence.
*   **Reusable Menu Shell**: Every host gets built-in `Settings`, `e-book`, `SDK Debug`, and `UI Gallery` tabs. Projects can register custom tabs and add their own sections to the built-in Settings tab.
*   **Generic UI Kit**: Theme tokens, layout helpers, compact tables, badges, status pills, meters, font/icon bootstrap, and drawlist helpers are available for host projects that want a polished ImGui surface without copying domain-specific widgets.
*   **Managed Floating Windows**: Register overlay windows with persisted open/pinned/position/size state and menu-bar pin controls.
*   **File-backed Diagnostics**: spdlog-backed logs are written under `%TEMP%\UniversalOverlay\<process-id>\` with category loggers for core, renderer, hooks, UI, windows, config, and related systems.
*   **Robust & Safe Unloading**: Multi-threaded reference counters track active hook execution threads to guarantee a safe, crash-free hook removal and DLL unloading sequence.
*   **Modern C++23**: Built on modern language features and practices.

---

## 🛠️ Architecture Overview

The codebase is organized logically into external dependencies and core library components:

```
UniversalOverlay/
├── External/                # Third-party libraries
│   ├── ImGui/               # Dear ImGui core & backends (Win32, DX9, DX11, DX12, OpenGL3)
│   ├── MinHook/             # MinHook headers & prebuilt static libraries (.lib)
│   └── spdlog/              # External/spdlog file-backed logging backend
├── Src/                     # Library implementation and public API
│   ├── UniversalOverlay.h   # Public umbrella include for consumers
│   ├── Core/                # Lifecycle, shared state, configuration, and logging
│   │   ├── UniversalOverlayCore.cpp
│   │   ├── CoreState.h/cpp
│   │   ├── ConfigSystem.h/cpp
│   │   ├── Log.h
│   │   └── OverlayLog.h/cpp
│   ├── Hooks/               # MinHook detours, WndProc handling, and cursor state hooks
│   │   ├── Hooks.h/cpp
│   │   ├── WndProcHook.h/cpp
│   │   └── CursorHook.h/cpp
│   ├── Renderer/            # Renderer facade, diagnostics, and backend contracts
│   │   ├── Renderer.h/cpp
│   │   ├── RendererBackend.h
│   │   ├── RendererDiagnostics.h
│   │   ├── RendererTypes.h
│   │   └── Backends/        # Renderer/Backends: OpenGL3, D3D9, guarded D3D10, D3D11, and D3D12
│   └── Ui/                  # Built-in menu, UI kit, gallery, and callback registry
│       ├── Menu.h/cpp
│       ├── UIRegistry.h/cpp
│       ├── OverlayTheme.h/cpp
│       ├── OverlayLayout.h/cpp
│       ├── OverlayDraw.h/cpp
│       ├── OverlayWidgets.h/cpp
│       ├── OverlayWindowManager.h/cpp
│       ├── OverlayFonts.h/cpp
│       ├── OverlayIcons.h/cpp
│       └── OverlayGallery.h/cpp
├── UniversalOverlay.sln     # Visual Studio 2022 Solution
└── UniversalOverlay.vcxproj # Project configuration file (C++23 static library target)
```

---

## 📦 Getting Started

### 1. Build the Static Library
1. Open `UniversalOverlay.sln` in **Visual Studio 2022**.
2. Select your configuration (**Debug** / **Release**) and platform target (**x86** / **x64**).
3. Build the project. The output `.lib` file will be generated in `Build/[Configuration]/[Platform]/UniversalOverlay.lib`.

### 2. Configure Your Project Linkage
To use UniversalOverlay in your DLL project:
*   Add `Src/` to your **Additional Include Directories**.
*   Add `External/ImGui`, `External/ImGui/Backends`, and `External/MinHook/include` to your includes.
*   Link against `UniversalOverlay.lib` and `libMinHook.[Platform].lib`.
*   Ensure your target project is configured to compile with the **C++23 Standard** (`/std:c++23`).

### 3. Basic Example (DLL Host Wrapper)
Here is a complete Direct3D 11 DLL wrapper that registers persistent settings, adds a custom Settings section, creates a tab, opens a managed floating window, and draws a tiny background overlay:

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
        ImGui::TextUnformatted("Example controls");
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
        sprintf_s(label, "UniversalOverlay example: %s", GetStatusText());

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

---

## 📖 API Reference

## Documentation

Wiki-style project notes live under `docs/wiki/`.

- `docs/wiki/Logging-and-Diagnostics.md` documents the spdlog-backed logging facade and temp-folder log layout.
- `docs/wiki/Third-Party-Dependencies.md` records pinned third-party dependencies, including the `External/spdlog` v1.17.0 submodule.

### Core Lifecycle
*   `bool Initialize(GraphicsAPI api)`: Installs Hooks and configures ImGui wrappers for the chosen API (`OpenGL3`, `D3D9`, `D3D10`, `D3D11`, `D3D12`). `D3D10` is currently exposed for compatibility and diagnostics, but runtime hook support returns false with a clear warning.
*   `void Shutdown()`: Safely restores original game code/hooks, cleans up ImGui descriptors, and releases windows handlers.
*   `bool IsInitialized()`: Checks if hooks are currently active.
*   `bool ShouldUnload()`: Becomes true when the configured unload module hotkey is pressed.
*   `void RequestUnload()`: Requests a clean unload, closes/disarms menu input, and lets the host thread finish shutdown.

### Menu Management
*   `bool IsMenuOpen()` / `void SetMenuOpen(bool open)`: Get or set the visibility state of the ImGui menu window.
*   `void SetMenuDefaultSize(float width, float height)`: Sets the first-use menu size while still letting saved config placement win.
*   `void RegisterTab(const std::string& name, TabCallback callback)`: Mounts a custom tab inside the main ImGui menu.
*   `void RegisterSettingsSection(const std::string& name, SettingsCallback callback)`: Adds project controls to the built-in `Settings` tab.
*   `void RegisterRenderCallback(RenderCallback callback)`: Register a callback to draw background elements directly onto the game window.
*   `void RegisterFloatingWindow(...)`: Register a managed ImGui window with saved open, pin, position, size, and background-alpha state.
*   `void SetFloatingWindowOpen(...)` / `void SetFloatingWindowPinned(...)`: Change managed window visibility and pin state.
*   `bool IsFloatingWindowOpen(...)` / `bool IsFloatingWindowPinned(...)`: Query managed window state.

### Configuration System
*   `void RegisterConfigBool(const std::string& section, const std::string& key, bool* val)`
*   `void RegisterConfigFloat(const std::string& section, const std::string& key, float* val, float defaultVal = 0.0f)`
*   `void RegisterConfigInt(const std::string& section, const std::string& key, int* val, int defaultVal = 0)`
*   `void MarkConfigDirty()`: Notify the config system that a UI interaction changed a registered value.
*   `void SaveConfig(const std::wstring& filePath)` / `void LoadConfig(const std::wstring& filePath)`: Manually save/load the state of registered configuration values, built-in theme values, main menu placement, and managed floating-window placement/open/pin state.
*   `bool SaveConfigPreset(int slot)` / `bool LoadConfigPreset(int slot)`: Save/load all registered values to one of five sibling preset files.
*   `std::wstring GetConfigPresetPath(int slot)`: Returns the on-disk path for a preset slot.
*   `bool SetConfigPresetName(int slot, const std::string& name)` / `const char* GetConfigPresetName(int slot)`: Edit or read the display name for a preset slot. Names are saved in the main config under `[PresetNames]`; preset snapshot filenames remain stable.

Built-in UI state is registered by `Initialize()`. New project options should register through the config API and call `MarkConfigDirty()` when their ImGui control returns changed.

---

## 📝 License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.
