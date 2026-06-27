#include "Ui/OverlayLayout.h"

#include "Ui/OverlayTheme.h"

namespace UniversalOverlay::Ui
{
    namespace
    {
        int g_sidebarDepth = 0;
        int g_tableDepth = 0;
        int g_tabBarDepth = 0;
        int g_sectionDepth = 0;
    }

    bool BeginSidebarLayout(const SidebarLayoutSpec& spec)
    {
        if (ImGui::GetCurrentContext() == nullptr)
            return false;

        const float width = spec.sidebarWidth > 0.0f ? spec.sidebarWidth : GetTheme().sidebarWidth;
        const ImGuiTableFlags flags =
            ImGuiTableFlags_SizingFixedFit |
            ImGuiTableFlags_NoSavedSettings |
            ImGuiTableFlags_BordersInnerV |
            ImGuiTableFlags_Resizable;

        const bool opened = ImGui::BeginTable(spec.id != nullptr ? spec.id : "SidebarLayout", 2, flags, spec.size);
        if (!opened)
            return false;

        ++g_sidebarDepth;
        ImGui::TableSetupColumn("Sidebar", ImGuiTableColumnFlags_WidthFixed, width);
        ImGui::TableSetupColumn("Content", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        return true;
    }

    bool BeginSidebarLayout(const char* id, float sidebarWidth)
    {
        SidebarLayoutSpec spec;
        spec.id = id;
        spec.sidebarWidth = sidebarWidth;
        return BeginSidebarLayout(spec);
    }

    void SidebarLayoutNextColumn()
    {
        if (g_sidebarDepth > 0)
            ImGui::TableSetColumnIndex(1);
    }

    void EndSidebarLayout()
    {
        if (g_sidebarDepth <= 0)
            return;

        --g_sidebarDepth;
        ImGui::EndTable();
    }

    bool BeginCompactTabBar(const char* id)
    {
        const bool opened = ImGui::BeginTabBar(id, ImGuiTabBarFlags_FittingPolicyScroll);
        if (opened)
            ++g_tabBarDepth;
        return opened;
    }

    void EndCompactTabBar()
    {
        if (g_tabBarDepth <= 0)
            return;

        --g_tabBarDepth;
        ImGui::EndTabBar();
    }

    bool BeginCompactTable(const char* id, int columns, ImGuiTableFlags flags)
    {
        if (columns <= 0)
            return false;

        const ImGuiTableFlags defaultFlags =
            ImGuiTableFlags_RowBg |
            ImGuiTableFlags_BordersInnerH |
            ImGuiTableFlags_SizingStretchProp |
            ImGuiTableFlags_NoSavedSettings;

        const bool opened = ImGui::BeginTable(id, columns, flags == 0 ? defaultFlags : flags);
        if (opened)
            ++g_tableDepth;
        return opened;
    }

    void EndCompactTable()
    {
        if (g_tableDepth <= 0)
            return;

        --g_tableDepth;
        ImGui::EndTable();
    }

    void SectionHeader(const char* label)
    {
        ImGui::Spacing();
        ImGui::SeparatorText(label != nullptr ? label : "");
    }

    bool BeginSection(const char* label)
    {
        SectionHeader(label);
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8.0f, 5.0f));
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ColorVec4(ThemeColor::SurfaceRaised, 0.28f));
        ImGui::BeginChild(label != nullptr ? label : "Section", ImVec2(0.0f, 0.0f), ImGuiChildFlags_FrameStyle | ImGuiChildFlags_AutoResizeY);
        ++g_sectionDepth;
        return true;
    }

    void EndSection()
    {
        if (g_sectionDepth <= 0)
            return;

        --g_sectionDepth;
        ImGui::EndChild();
        ImGui::PopStyleColor();
        ImGui::PopStyleVar();
    }
}
