#pragma once

#include "UniversalOverlay.h"
#include <string>
#include <vector>

namespace UniversalOverlay
{
    struct MenuTab
    {
        std::string name;
        TabCallback callback;
    };

    namespace UIRegistry
    {
        void RegisterTab(const std::string& name, TabCallback callback);
        const std::vector<MenuTab>& GetTabs();
        void Clear();
    }
}
