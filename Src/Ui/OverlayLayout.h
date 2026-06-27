#pragma once

#include "imgui.h"

namespace UniversalOverlay::Ui
{
    struct SidebarLayoutSpec
    {
        const char* id = "SidebarLayout";
        float sidebarWidth = 0.0f;
        ImVec2 size = ImVec2(0.0f, 0.0f);
    };

    bool BeginSidebarLayout(const SidebarLayoutSpec& spec);
    bool BeginSidebarLayout(const char* id, float sidebarWidth = 0.0f);
    void SidebarLayoutNextColumn();
    void EndSidebarLayout();

    bool BeginCompactTabBar(const char* id);
    void EndCompactTabBar();
    bool BeginCompactTable(const char* id, int columns, ImGuiTableFlags flags = 0);
    void EndCompactTable();
    void SectionHeader(const char* label);
    bool BeginSection(const char* label);
    void EndSection();
}
