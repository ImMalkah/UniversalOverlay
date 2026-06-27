#include "Ui/UIRegistry.h"

#include "Ui/OverlayWindowManager.h"

#include <utility>

namespace UniversalOverlay
{
    namespace UIRegistry
    {
        static std::vector<MenuTab> g_tabs;

        void RegisterTab(const std::string& name, TabCallback callback)
        {
            for (MenuTab& tab : g_tabs)
            {
                if (tab.name == name)
                {
                    tab.callback = std::move(callback);
                    return;
                }
            }

            g_tabs.push_back({ name, std::move(callback) });
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
            Ui::ManagedWindowSpec spec;
            spec.name = name;
            spec.callback = std::move(callback);
            spec.defaultOpen = defaultOpen;
            spec.defaultPinned = defaultPinned;
            spec.backgroundAlpha = backgroundAlpha;
            Ui::RegisterWindow(spec);
        }

        void SetFloatingWindowOpen(const std::string& name, bool open)
        {
            Ui::SetOpen(name, open);
        }

        void SetFloatingWindowPinned(const std::string& name, bool pinned)
        {
            Ui::SetPinned(name, pinned);
        }

        bool IsFloatingWindowOpen(const std::string& name)
        {
            return Ui::IsOpen(name);
        }

        bool IsFloatingWindowPinned(const std::string& name)
        {
            return Ui::IsPinned(name);
        }

        void DrawFloatingWindows(bool menuOpen)
        {
            Ui::DrawWindows(menuOpen);
        }

        void Clear()
        {
            g_tabs.clear();
            Ui::ClearWindows();
        }
    }
}
