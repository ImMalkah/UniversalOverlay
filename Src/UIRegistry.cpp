#include "UIRegistry.h"

namespace UniversalOverlay
{
    namespace UIRegistry
    {
        static std::vector<MenuTab> g_tabs;

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

        void Clear()
        {
            g_tabs.clear();
        }
    }
}
