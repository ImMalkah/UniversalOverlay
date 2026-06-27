#include "Ui/OverlayWindowManager.h"

#include "Core/ConfigSystem.h"
#include "Core/OverlayLog.h"

#include <algorithm>
#include <cmath>
#include <deque>
#include <utility>

namespace UniversalOverlay::Ui
{
    namespace
    {
        struct ManagedWindow
        {
            ManagedWindowSpec spec;
            ManagedWindowState state;
        };

        std::deque<ManagedWindow> g_windows;
        bool g_registeredPostLoadCallback = false;

        bool HasMeaningfulDelta(float lhs, float rhs)
        {
            return std::fabs(lhs - rhs) > 0.5f;
        }

        std::string WindowConfigSection(const std::string& name)
        {
            std::string section = "Window.";
            section.reserve(section.size() + name.size());

            for (char ch : name)
            {
                if (ch == '[' || ch == ']' || ch == '=' || ch == '\r' || ch == '\n')
                    section.push_back('_');
                else
                    section.push_back(ch);
            }

            return section;
        }

        void RegisterWindowStateConfig(ManagedWindowState& state)
        {
            if (!g_registeredPostLoadCallback)
            {
                ConfigSystem::RegisterPostLoadCallback("UniversalOverlay.ManagedWindows", []()
                {
                    for (ManagedWindow& window : g_windows)
                        window.state.applySavedPlacement = true;
                });
                g_registeredPostLoadCallback = true;
            }

            const std::string section = WindowConfigSection(state.name);
            ConfigSystem::RegisterBool(section, "Open", &state.open, state.open);
            ConfigSystem::RegisterBool(section, "Pinned", &state.pinned, state.pinned);
            ConfigSystem::RegisterFloat(section, "BackgroundAlpha", &state.backgroundAlpha, state.backgroundAlpha);
            ConfigSystem::RegisterFloat(section, "PositionX", &state.position.x, state.position.x);
            ConfigSystem::RegisterFloat(section, "PositionY", &state.position.y, state.position.y);
            ConfigSystem::RegisterFloat(section, "Width", &state.size.x, state.size.x);
            ConfigSystem::RegisterFloat(section, "Height", &state.size.y, state.size.y);

            if (ConfigSystem::IsConfigLoaded())
                state.applySavedPlacement = true;
        }

        ManagedWindow* FindWindow(const std::string& name)
        {
            for (ManagedWindow& window : g_windows)
            {
                if (window.state.name == name)
                    return &window;
            }

            return nullptr;
        }

        const ManagedWindow* FindWindowConst(const std::string& name)
        {
            for (const ManagedWindow& window : g_windows)
            {
                if (window.state.name == name)
                    return &window;
            }

            return nullptr;
        }

        ManagedWindowState BuildState(const ManagedWindowSpec& spec)
        {
            ManagedWindowState state;
            state.name = spec.name;
            state.open = spec.defaultOpen;
            state.pinned = spec.defaultPinned;
            state.backgroundAlpha = spec.backgroundAlpha;
            state.size = spec.defaultSize;
            state.position = spec.defaultPosition;
            return state;
        }
    }

    void RegisterWindow(const ManagedWindowSpec& spec)
    {
        if (spec.name.empty())
        {
            Log::Warn(Log::LogCategory::Windows, "ignored managed window registration with empty name");
            return;
        }

        if (ManagedWindow* existing = FindWindow(spec.name))
        {
            existing->spec = spec;
            existing->state.backgroundAlpha = spec.backgroundAlpha;
            if (existing->state.size.x <= 0.0f || existing->state.size.y <= 0.0f)
                existing->state.size = spec.defaultSize;
            if (existing->state.position.x == 0.0f && existing->state.position.y == 0.0f)
                existing->state.position = spec.defaultPosition;
            RegisterWindowStateConfig(existing->state);
            Log::Info(Log::LogCategory::Windows, "managed window registration replaced");
            return;
        }

        ManagedWindow window;
        window.spec = spec;
        window.state = BuildState(spec);
        g_windows.push_back(std::move(window));
        RegisterWindowStateConfig(g_windows.back().state);
        Log::Info(Log::LogCategory::Windows, "managed window registered");
    }

    void SetOpen(const std::string& name, bool open)
    {
        if (ManagedWindow* window = FindWindow(name))
        {
            if (window->state.open != open)
                ConfigSystem::MarkDirty();
            window->state.open = open;
        }
    }

    void SetPinned(const std::string& name, bool pinned)
    {
        if (ManagedWindow* window = FindWindow(name))
        {
            if (window->state.pinned != pinned)
                ConfigSystem::MarkDirty();
            window->state.pinned = pinned;
        }
    }

    bool IsOpen(const std::string& name)
    {
        if (const ManagedWindow* window = FindWindowConst(name))
            return window->state.open;

        return false;
    }

    bool IsPinned(const std::string& name)
    {
        if (const ManagedWindow* window = FindWindowConst(name))
            return window->state.pinned;

        return false;
    }

    const ManagedWindowState* GetWindowState(const std::string& name)
    {
        if (const ManagedWindow* window = FindWindowConst(name))
            return &window->state;

        return nullptr;
    }

    const std::vector<ManagedWindowState>& GetWindowStates()
    {
        static std::vector<ManagedWindowState> states;
        states.clear();
        states.reserve(g_windows.size());

        for (const ManagedWindow& window : g_windows)
            states.push_back(window.state);

        return states;
    }

    void DrawWindows(bool menuOpen)
    {
        for (ManagedWindow& window : g_windows)
        {
            window.state.acceptingInput = false;

            if (!window.state.open || !(menuOpen || window.state.pinned))
                continue;

            window.state.acceptingInput = menuOpen;

            ImGuiWindowFlags flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_MenuBar;
            if (!menuOpen)
                flags |= ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize;

            const ImGuiCond placementCondition = window.state.applySavedPlacement ? ImGuiCond_Always : ImGuiCond_FirstUseEver;
            ImGui::SetNextWindowBgAlpha(std::clamp(window.state.backgroundAlpha, 0.15f, 1.0f));
            ImGui::SetNextWindowSize(window.state.size, placementCondition);
            ImGui::SetNextWindowPos(window.state.position, placementCondition);

            bool open = window.state.open;
            if (ImGui::Begin(window.state.name.c_str(), &open, flags))
            {
                if (ImGui::BeginMenuBar())
                {
                    const bool pinned = window.state.pinned;
                    if (ImGui::MenuItem(pinned ? "Pinned" : "Pin", nullptr, pinned))
                    {
                        SetPinned(window.state.name, !pinned);
                    }
                    ImGui::EndMenuBar();
                }

                if (window.spec.callback)
                    window.spec.callback(menuOpen);

                const ImVec2 position = ImGui::GetWindowPos();
                const ImVec2 size = ImGui::GetWindowSize();
                if (HasMeaningfulDelta(window.state.position.x, position.x) ||
                    HasMeaningfulDelta(window.state.position.y, position.y) ||
                    HasMeaningfulDelta(window.state.size.x, size.x) ||
                    HasMeaningfulDelta(window.state.size.y, size.y))
                {
                    window.state.position = position;
                    window.state.size = size;
                    ConfigSystem::MarkDirty();
                }
                window.state.lastDrawTime = ImGui::GetTime();
            }

            ImGui::End();
            window.state.applySavedPlacement = false;
            if (window.state.open != open)
                ConfigSystem::MarkDirty();
            window.state.open = open;
        }
    }

    void ClearWindows()
    {
        g_windows.clear();
    }
}
