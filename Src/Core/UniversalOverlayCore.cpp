#include "UniversalOverlay.h"
#include "Core/CoreState.h"
#include <MinHook.h>
#include "Renderer/Renderer.h"
#include "Hooks/Hooks.h"
#include "Ui/UIRegistry.h"
#include "Ui/OverlayTheme.h"
#include "Core/ConfigSystem.h"
#include "Core/Log.h"
#include "Core/OverlayLog.h"

#include <utility>

namespace UniversalOverlay
{
    bool Initialize(GraphicsAPI api)
    {
        if (State::initialized)
            return true;

        Log::InitializeLogging(L"UniversalOverlay");

        Log::Debug("Initializing SDK framework...");

        // Register SDK internal hotkeys to the config manager
        RegisterConfigInt("Keys", "MenuToggle", &State::menuToggleKey, VK_OEM_PERIOD);
        RegisterConfigInt("Keys", "Unload", &State::unloadKey, VK_F1);
        RegisterConfigBool("Menu", "Open", &State::menuOpen);
        RegisterConfigFloat("Menu", "PositionX", &State::menuPositionX, 80.0f);
        RegisterConfigFloat("Menu", "PositionY", &State::menuPositionY, 80.0f);
        RegisterConfigFloat("Menu", "Width", &State::menuSizeX, 550.0f);
        RegisterConfigFloat("Menu", "Height", &State::menuSizeY, 380.0f);
        ConfigSystem::RegisterPostLoadCallback("UniversalOverlay.MenuPlacement", []()
        {
            State::applySavedMenuPlacement = true;
        });
        Ui::RegisterThemeConfig();

        if (!Hooks::Install(api))
        {
            Log::Debug("Failed to install hooks.");
            Hooks::Remove();
            Log::ShutdownLogging();
            return false;
        }

        State::initialized = true;
        Log::Debug("SDK initialized successfully.");
        return true;
    }

    void Shutdown()
    {
        if (!State::initialized)
            return;

        Log::Debug("Shutting down SDK framework...");

        Hooks::Remove();
        Renderer::Shutdown();
        UIRegistry::Clear();
        ConfigSystem::Clear();

        State::initialized = false;
        Log::Debug("SDK shutdown complete.");
        Log::ShutdownLogging();
    }

    bool IsInitialized()
    {
        return State::initialized;
    }

    bool ShouldUnload()
    {
        return State::shouldUnload;
    }

    void RequestUnload()
    {
        const bool alreadyRequested = State::shouldUnload.exchange(true);
        if (!alreadyRequested)
            Log::Debug("Unload requested.");

        if (State::menuOpen)
            ConfigSystem::MarkDirty();

        State::menuOpen = false;
        State::waitingForMenuToggleKey = false;
        State::waitingForUnloadKey = false;
        State::keyCaptureWaitingForRelease = false;
        Renderer::SetDrawCursor(false);
    }

    bool IsMenuOpen()
    {
        return State::menuOpen;
    }

    void SetMenuOpen(bool open)
    {
        if (State::menuOpen != open)
            ConfigSystem::MarkDirty();
        State::menuOpen = open;
    }

    void SetMenuDefaultSize(float width, float height)
    {
        const float safeWidth = width > 320.0f ? width : 320.0f;
        const float safeHeight = height > 240.0f ? height : 240.0f;
        ConfigSystem::RegisterFloat("Menu", "Width", &State::menuSizeX, safeWidth);
        ConfigSystem::RegisterFloat("Menu", "Height", &State::menuSizeY, safeHeight);
    }

    HWND GetGameWindow()
    {
        return State::windowHandle;
    }

    void RegisterTab(const std::string& name, TabCallback callback)
    {
        UIRegistry::RegisterTab(name, callback);
    }

    void RegisterSettingsSection(const std::string& name, SettingsCallback callback)
    {
        UIRegistry::RegisterSettingsSection(name, std::move(callback));
    }

    void RegisterRenderCallback(RenderCallback callback)
    {
        State::renderCallback = callback;
    }

    void RegisterFloatingWindow(
        const std::string& name,
        FloatingWindowCallback callback,
        bool defaultOpen,
        bool defaultPinned,
        float backgroundAlpha)
    {
        UIRegistry::RegisterFloatingWindow(name, std::move(callback), defaultOpen, defaultPinned, backgroundAlpha);
    }

    void SetFloatingWindowOpen(const std::string& name, bool open)
    {
        UIRegistry::SetFloatingWindowOpen(name, open);
    }

    void SetFloatingWindowPinned(const std::string& name, bool pinned)
    {
        UIRegistry::SetFloatingWindowPinned(name, pinned);
    }

    bool IsFloatingWindowOpen(const std::string& name)
    {
        return UIRegistry::IsFloatingWindowOpen(name);
    }

    bool IsFloatingWindowPinned(const std::string& name)
    {
        return UIRegistry::IsFloatingWindowPinned(name);
    }

    void DrawFloatingWindows(bool menuOpen)
    {
        UIRegistry::DrawFloatingWindows(menuOpen);
    }

    void RegisterConfigBool(const std::string& section, const std::string& key, bool* val)
    {
        ConfigSystem::RegisterBool(section, key, val, *val);
    }

    void RegisterConfigFloat(const std::string& section, const std::string& key, float* val, float defaultVal)
    {
        ConfigSystem::RegisterFloat(section, key, val, defaultVal);
    }

    void RegisterConfigInt(const std::string& section, const std::string& key, int* val, int defaultVal)
    {
        ConfigSystem::RegisterInt(section, key, val, defaultVal);
    }

    void MarkConfigDirty()
    {
        ConfigSystem::MarkDirty();
    }

    void SaveConfig(const std::wstring& filePath)
    {
        ConfigSystem::Save(filePath);
    }

    void LoadConfig(const std::wstring& filePath)
    {
        ConfigSystem::Load(filePath);
    }

    bool SaveConfigPreset(int slot)
    {
        return ConfigSystem::SavePreset(slot);
    }

    bool LoadConfigPreset(int slot)
    {
        return ConfigSystem::LoadPreset(slot);
    }

    std::wstring GetConfigPresetPath(int slot)
    {
        return ConfigSystem::GetPresetPath(slot);
    }

    bool CreateHook(void* target, void* detour, void** original)
    {
        return MH_CreateHook(target, detour, original) == MH_OK;
    }

    bool RemoveHook(void* target)
    {
        return MH_RemoveHook(target) == MH_OK;
    }

    bool EnableHook(void* target)
    {
        return MH_EnableHook(target) == MH_OK;
    }

    bool DisableHook(void* target)
    {
        return MH_DisableHook(target) == MH_OK;
    }
}
