#include "Ui/OverlayWindowManager.h"

#include "Core/OverlayLog.h"

#include <algorithm>
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

        std::vector<ManagedWindow> g_windows;

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
            Log::Info(Log::LogCategory::Windows, "managed window registration replaced");
            return;
        }

        ManagedWindow window;
        window.spec = spec;
        window.state = BuildState(spec);
        g_windows.push_back(std::move(window));
        Log::Info(Log::LogCategory::Windows, "managed window registered");
    }

    void SetOpen(const std::string& name, bool open)
    {
        if (ManagedWindow* window = FindWindow(name))
            window->state.open = open;
    }

    void SetPinned(const std::string& name, bool pinned)
    {
        if (ManagedWindow* window = FindWindow(name))
            window->state.pinned = pinned;
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

            ImGui::SetNextWindowBgAlpha(std::clamp(window.state.backgroundAlpha, 0.15f, 1.0f));
            ImGui::SetNextWindowSize(window.state.size, ImGuiCond_FirstUseEver);
            ImGui::SetNextWindowPos(window.state.position, ImGuiCond_FirstUseEver);

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

                window.state.position = ImGui::GetWindowPos();
                window.state.size = ImGui::GetWindowSize();
                window.state.lastDrawTime = ImGui::GetTime();
            }

            ImGui::End();
            window.state.open = open;
        }
    }

    void ClearWindows()
    {
        g_windows.clear();
    }
}
