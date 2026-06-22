#include "UIRegistry.h"

#include "imgui.h"

#include <algorithm>
#include <utility>

namespace UniversalOverlay
{
    namespace UIRegistry
    {
        static std::vector<MenuTab> g_tabs;
        static std::vector<FloatingWindow> g_floatingWindows;

        void RegisterTab(const std::string& name, TabCallback callback)
        {
            // Avoid duplicates
            for (auto& tab : g_tabs)
            {
                if (tab.name == name)
                {
                    tab.callback = callback;
                    return;
                }
            }

            g_tabs.push_back({ name, callback });
        }

        const std::vector<MenuTab>& GetTabs()
        {
            return g_tabs;
        }

        void RegisterFloatingWindow(
            const std::string& name,
            FloatingWindowCallback callback,
            bool defaultOpen,
            bool defaultPinned,
            float backgroundAlpha)
        {
            for (FloatingWindow& window : g_floatingWindows)
            {
                if (window.name == name)
                {
                    window.callback = std::move(callback);
                    window.backgroundAlpha = backgroundAlpha;
                    return;
                }
            }

            g_floatingWindows.push_back({ name, std::move(callback), defaultOpen, defaultPinned, backgroundAlpha });
        }

        FloatingWindow* FindFloatingWindow(const std::string& name)
        {
            for (FloatingWindow& window : g_floatingWindows)
            {
                if (window.name == name)
                    return &window;
            }

            return nullptr;
        }

        void SetFloatingWindowOpen(const std::string& name, bool open)
        {
            if (FloatingWindow* window = FindFloatingWindow(name))
                window->open = open;
        }

        void SetFloatingWindowPinned(const std::string& name, bool pinned)
        {
            if (FloatingWindow* window = FindFloatingWindow(name))
                window->pinned = pinned;
        }

        bool IsFloatingWindowOpen(const std::string& name)
        {
            if (const FloatingWindow* window = FindFloatingWindow(name))
                return window->open;

            return false;
        }

        bool IsFloatingWindowPinned(const std::string& name)
        {
            if (const FloatingWindow* window = FindFloatingWindow(name))
                return window->pinned;

            return false;
        }

        void DrawFloatingWindows(bool menuOpen)
        {
            for (FloatingWindow& window : g_floatingWindows)
            {
                if (!window.open || !(menuOpen || window.pinned))
                    continue;

                ImGuiWindowFlags flags = ImGuiWindowFlags_NoCollapse;
                if (!menuOpen)
                    flags |= ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize;

                ImGui::SetNextWindowBgAlpha(std::clamp(window.backgroundAlpha, 0.15f, 1.0f));

                bool open = window.open;
                if (ImGui::Begin(window.name.c_str(), &open, flags))
                {
                    if (window.callback)
                        window.callback(menuOpen);
                }
                ImGui::End();

                window.open = open;
            }
        }

        void Clear()
        {
            g_tabs.clear();
            g_floatingWindows.clear();
        }
    }
}
