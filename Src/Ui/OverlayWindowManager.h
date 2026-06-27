#pragma once

#include "imgui.h"

#include <functional>
#include <string>
#include <vector>

namespace UniversalOverlay::Ui
{
    using ManagedWindowCallback = std::function<void(bool menuOpen)>;

    struct ManagedWindowSpec
    {
        std::string name;
        ManagedWindowCallback callback;
        bool defaultOpen = false;
        bool defaultPinned = false;
        float backgroundAlpha = 0.55f;
        ImVec2 defaultSize = ImVec2(360.0f, 240.0f);
        ImVec2 defaultPosition = ImVec2(80.0f, 80.0f);
    };

    struct ManagedWindowState
    {
        std::string name;
        bool open = false;
        bool pinned = false;
        bool acceptingInput = false;
        float backgroundAlpha = 0.55f;
        ImVec2 size = ImVec2(0.0f, 0.0f);
        ImVec2 position = ImVec2(0.0f, 0.0f);
        bool applySavedPlacement = false;
        double lastDrawTime = 0.0;
    };

    void RegisterWindow(const ManagedWindowSpec& spec);
    void SetOpen(const std::string& name, bool open);
    void SetPinned(const std::string& name, bool pinned);
    bool IsOpen(const std::string& name);
    bool IsPinned(const std::string& name);
    const ManagedWindowState* GetWindowState(const std::string& name);
    const std::vector<ManagedWindowState>& GetWindowStates();
    void DrawWindows(bool menuOpen);
    void ClearWindows();
}
