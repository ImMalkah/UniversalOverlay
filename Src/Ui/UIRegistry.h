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

    struct SettingsSection
    {
        std::string name;
        SettingsCallback callback;
    };

    namespace UIRegistry
    {
        void RegisterTab(const std::string& name, TabCallback callback);
        const std::vector<MenuTab>& GetTabs();

        void RegisterSettingsSection(const std::string& name, SettingsCallback callback);
        const std::vector<SettingsSection>& GetSettingsSections();

        void RegisterFloatingWindow(
            const std::string& name,
            FloatingWindowCallback callback,
            bool defaultOpen,
            bool defaultPinned,
            float backgroundAlpha);
        void SetFloatingWindowOpen(const std::string& name, bool open);
        void SetFloatingWindowPinned(const std::string& name, bool pinned);
        bool IsFloatingWindowOpen(const std::string& name);
        bool IsFloatingWindowPinned(const std::string& name);
        void DrawFloatingWindows(bool menuOpen);

        void Clear();
    }
}
