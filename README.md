# UniversalOverlay

[![C++ Standard](https://img.shields.io/badge/C%2B%2B-23-blue.svg?style=flat-square&logo=cplusplus)](https://en.cppreference.com/w/cpp/compiler_support/23)
[![Platform](https://img.shields.io/badge/Platform-Windows-lightgrey.svg?style=flat-square&logo=windows)](https://docs.microsoft.com/en-us/windows/)
[![Graphics APIs](https://img.shields.io/badge/APIs-DX9%20%7C%20DX11%20%7C%20DX12%20%7C%20OpenGL3-orange.svg?style=flat-square)](https://github.com/ocornut/imgui)
[![License](https://img.shields.io/badge/License-MIT-green.svg?style=flat-square)](https://opensource.org/licenses/MIT)

**UniversalOverlay** is a lightweight, modern, and highly modular C++23 static library designed to hook into target processes (games or 3D applications) and inject a custom overlay menu and rendering callbacks. 

Powered by **Dear ImGui** and **MinHook**, it intercepts render loops, hijacks window input (`WndProc`), manages cursor locking state, and provides a built-in INI configuration system with keybinding collision resolution.

---

## 🚀 Key Features

*   **Multi-API Support**: Out-of-the-box detours for:
    *   **OpenGL 3** (detouring `wglSwapBuffers`)
    *   **Direct3D 9** (detouring vtable `EndScene` & `Reset`)
    *   **Direct3D 11** (detouring vtable `Present` & `ResizeBuffers`)
    *   **Direct3D 12** (detouring vtable `Present`, `ResizeBuffers`, & command queue `ExecuteCommandLists`)
*   **Dynamic VTable Resolution**: Avoids hardcoded virtual table offsets by instantiating temporary dummy windows and graphics devices at startup to resolve runtime function addresses.
*   **Seamless Input Hijacking**: Hooked `WndProc` redirects keyboard and mouse inputs to ImGui. Cursor hooks (`SetCursor`, `ClipCursor`, `ShowCursor`) are automatically detoured to release/re-lock the cursor when the menu state changes.
*   **Built-in Configuration System**: Easily register `bool`, `float`, and `int` settings. Features automated disk serialization/deserialization to INI files and interactive menu keybind capturing.
*   **Robust & Safe Unloading**: Multi-threaded reference counters track active hook execution threads to guarantee a safe, crash-free hook removal and DLL unloading sequence.
*   **Modern C++23**: Built on modern language features and practices.

---

## 🛠️ Architecture Overview

The codebase is organized logically into external dependencies and core library components:

```
UniversalOverlay/
├── External/                # Third-party libraries
│   ├── ImGui/               # Dear ImGui core & backends (Win32, DX9, DX11, DX12, OpenGL3)
│   └── MinHook/             # MinHook headers & prebuilt static libraries (.lib)
├── Src/                     # Library implementation and public API
│   ├── UniversalOverlay.h   # Public umbrella include for consumers
│   ├── Core/                # Lifecycle, shared state, configuration, and logging
│   │   ├── UniversalOverlayCore.cpp
│   │   ├── CoreState.h/cpp
│   │   ├── ConfigSystem.h/cpp
│   │   └── Log.h
│   ├── Hooks/               # MinHook detours, WndProc handling, and cursor state hooks
│   │   ├── Hooks.h/cpp
│   │   ├── WndProcHook.h/cpp
│   │   └── CursorHook.h/cpp
│   ├── Renderer/            # ImGui backend context initialization per graphics API
│   │   ├── Renderer.h/cpp
│   │   └── Backends/        # Reserved for renderer backend-specific source organization
│   └── Ui/                  # Built-in menu layout and UI callback registry
│       ├── Menu.h/cpp
│       └── UIRegistry.h/cpp
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

### 3. Basic Example (DLL Injection Wrapper)
Here is how you can write a simple DLL that injects into a target DirectX 11 game, creates a custom tab, and sets up ESP-rendering callbacks:

```cpp
#include <Windows.h>
#include <UniversalOverlay.h>
#include <imgui.h>

// 1. Define custom features
bool g_esp_enabled = false;
float g_esp_distance = 500.0f;
int g_esp_color = 0xFFFFFFFF; // White

// 2. Define custom Menu tab drawing callback
void RenderESPConfigTab()
{
    ImGui::Text("ESP Configuration Settings");
    ImGui::Separator();
    
    ImGui::Checkbox("Enable ESP Rendering", &g_esp_enabled);
    ImGui::SliderFloat("Max Distance (m)", &g_esp_distance, 10.0f, 1000.0f, "%.0fm");
    
    // Config values are automatically bound and will save when modified!
}

// 3. Define raw drawing callback (executed on every frame render loop)
void DrawESPOverlay()
{
    if (!g_esp_enabled)
        return;

    ImDrawList* drawList = ImGui::GetBackgroundDrawList();
    
    // Draw ESP lines, boxes, or indicators directly onto the screen background
    drawList->AddText(ImVec2(20.0f, 20.0f), IM_COL32(0, 255, 0, 255), "ESP Active - UniversalOverlay");
    
    char distBuf[64];
    sprintf_s(distBuf, "Tracking up to: %.0fm", g_esp_distance);
    drawList->AddText(ImVec2(20.0f, 40.0f), IM_COL32(255, 255, 255, 200), distBuf);
}

// 4. Main background thread running after DLL injection
DWORD WINAPI MainThread(LPVOID lpParam)
{
    // Initialize the framework for DirectX 11
    if (!UniversalOverlay::Initialize(UniversalOverlay::GraphicsAPI::D3D11))
    {
        return 0;
    }

    // Register configurations with the automated config manager
    // Pass absolute paths or prefix with ".\\" (e.g. L".\\my_config.ini") to prevent savings in System32!
    std::wstring configPath = L".\\UniversalOverlaySettings.ini";
    
    UniversalOverlay::RegisterConfigBool("Visuals", "EspEnabled", &g_esp_enabled);
    UniversalOverlay::RegisterConfigFloat("Visuals", "EspMaxDistance", &g_esp_distance, 500.0f);
    
    // Load config if it exists
    UniversalOverlay::LoadConfig(configPath);

    // Register our UI components
    UniversalOverlay::RegisterTab("Visuals", RenderESPConfigTab);
    UniversalOverlay::RegisterRenderCallback(DrawESPOverlay);

    // Loop until the Unload key is pressed (defaults to END key)
    while (!UniversalOverlay::ShouldUnload())
    {
        Sleep(100);
    }

    // Save configuration settings
    UniversalOverlay::SaveConfig(configPath);

    // Safely remove hooks, restore WndProc, release cursor capture and exit
    UniversalOverlay::Shutdown();
    
    FreeLibraryAndExitThread(static_cast<HMODULE>(lpParam), 0);
    return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    if (ul_reason_for_call == DLL_PROCESS_ATTACH)
    {
        DisableThreadLibraryCalls(hModule);
        CloseHandle(CreateThread(nullptr, 0, MainThread, hModule, 0, nullptr));
    }
    return TRUE;
}
```

---

## 📖 API Reference

### Core Lifecycle
*   `bool Initialize(GraphicsAPI api)`: Installs Hooks and configures ImGui wrappers for the chosen API (`OpenGL3`, `D3D9`, `D3D11`, `D3D12`).
*   `void Shutdown()`: Safely restores original game code/hooks, cleans up ImGui descriptors, and releases windows handlers.
*   `bool IsInitialized()`: Checks if hooks are currently active.
*   `bool ShouldUnload()`: Becomes true when the configured unload module hotkey is pressed.

### Menu Management
*   `bool IsMenuOpen()` / `void SetMenuOpen(bool open)`: Get or set the visibility state of the ImGui menu window.
*   `void RegisterTab(const std::string& name, TabCallback callback)`: Mounts a custom tab inside the main ImGui menu.
*   `void RegisterRenderCallback(RenderCallback callback)`: Register a callback to draw background elements directly onto the game window.

### Configuration System
*   `void RegisterConfigBool(const std::string& section, const std::string& key, bool* val)`
*   `void RegisterConfigFloat(const std::string& section, const std::string& key, float* val, float defaultVal = 0.0f)`
*   `void RegisterConfigInt(const std::string& section, const std::string& key, int* val, int defaultVal = 0)`
*   `void SaveConfig(const std::wstring& filePath)` / `void LoadConfig(const std::wstring& filePath)`: Manually save/load the state of registered configuration values.

---

## 📝 License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.
