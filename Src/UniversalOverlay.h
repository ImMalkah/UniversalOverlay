#pragma once

#include <Windows.h>
#include <functional>
#include <string>
#include <vector>

#include "Renderer/RendererTypes.h"
#include "Ui/OverlayWindowManager.h"

#if defined(_M_X64)
#pragma comment(lib, "libMinHook.x64.lib")
#elif defined(_M_IX86)
#pragma comment(lib, "libMinHook.x86.lib")
#endif

namespace UniversalOverlay
{
    // Initialize the overlay framework for the given Graphics API
    bool Initialize(GraphicsAPI api);

    // Shutdown the overlay framework (uninstalls hooks, cleans up ImGui)
    void Shutdown();

    // Check if the overlay is initialized
    bool IsInitialized();

    // Check if the DLL should unload
    bool ShouldUnload();

    // Check if the menu is currently open/visible
    bool IsMenuOpen();

    // Set the menu open/visible state
    void SetMenuOpen(bool open);

    // Override the first-use menu size before or after config load. Saved
    // config values still win when they exist.
    void SetMenuDefaultSize(float width, float height);

    // Get the window handle of the game
    HWND GetGameWindow();

    // Register a tab in the menu
    using TabCallback = std::function<void()>;
    void RegisterTab(const std::string& name, TabCallback callback);

    // Register a section inside the built-in Settings tab. Use RegisterConfig*
    // for values drawn here so normal config save/load and presets include them.
    using SettingsCallback = std::function<void()>;
    void RegisterSettingsSection(const std::string& name, SettingsCallback callback);

    // Register a general rendering callback (called on every frame, e.g. for ESP background drawing)
    using RenderCallback = std::function<void()>;
    void RegisterRenderCallback(RenderCallback callback);

    // Register a named ImGui floating window. Floating windows are rendered every
    // frame; unpinned windows are only visible while the menu is open.
    using FloatingWindowCallback = std::function<void(bool menuOpen)>;
    void RegisterFloatingWindow(
        const std::string& name,
        FloatingWindowCallback callback,
        bool defaultOpen = false,
        bool defaultPinned = false,
        float backgroundAlpha = 0.55f);
    void SetFloatingWindowOpen(const std::string& name, bool open);
    void SetFloatingWindowPinned(const std::string& name, bool pinned);
    bool IsFloatingWindowOpen(const std::string& name);
    bool IsFloatingWindowPinned(const std::string& name);
    void DrawFloatingWindows(bool menuOpen);

    // Config management API
    void RegisterConfigBool(const std::string& section, const std::string& key, bool* val);
    void RegisterConfigFloat(const std::string& section, const std::string& key, float* val, float defaultVal = 0.0f);
    void RegisterConfigInt(const std::string& section, const std::string& key, int* val, int defaultVal = 0);
    void MarkConfigDirty();

    // Save and load configurations manually
    void SaveConfig(const std::wstring& filePath);
    void LoadConfig(const std::wstring& filePath);
    bool SaveConfigPreset(int slot);
    bool LoadConfigPreset(int slot);
    std::wstring GetConfigPresetPath(int slot);

    // Capture/Keyboard keybind helpers
    int GetPressedKey();
    const char* GetKeyName(int key);

    // Modifier masks encoded in the key value
    constexpr int HOTKEY_MOD_CTRL = 1 << 16;
    constexpr int HOTKEY_MOD_SHIFT = 1 << 17;
    constexpr int HOTKEY_MOD_ALT = 1 << 18;

    // Helper functions
    bool IsHotkeyComboPressed(int keybindCode);
    std::string GetHotkeyComboName(int keybindCode);

    // Hooking wrappers
    bool CreateHook(void* target, void* detour, void** original);
    bool RemoveHook(void* target);
    bool EnableHook(void* target);
    bool DisableHook(void* target);
}
