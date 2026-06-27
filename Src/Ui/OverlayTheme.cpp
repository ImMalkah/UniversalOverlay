#include "Ui/OverlayTheme.h"

#include <algorithm>

namespace UniversalOverlay::Ui
{
    namespace
    {
        OverlayTheme g_theme = MakeTacticalDarkTheme();

        constexpr int ThemeColorIndex(ThemeColor color)
        {
            return static_cast<int>(color);
        }

        ImVec4 WithAlpha(ImVec4 color, float alphaScale)
        {
            color.w = std::clamp(color.w * alphaScale, 0.0f, 1.0f);
            return color;
        }

        ImVec4 LerpColor(const ImVec4& a, const ImVec4& b, float t)
        {
            t = std::clamp(t, 0.0f, 1.0f);
            return ImVec4(
                a.x + ((b.x - a.x) * t),
                a.y + ((b.y - a.y) * t),
                a.z + ((b.z - a.z) * t),
                a.w + ((b.w - a.w) * t));
        }

        void SetColor(OverlayTheme& theme, ThemeColor color, ImVec4 value)
        {
            theme.colors[ThemeColorIndex(color)] = value;
        }
    }

    OverlayTheme MakeTacticalDarkTheme()
    {
        OverlayTheme theme;
        SetColor(theme, ThemeColor::Text, ImVec4(0.92f, 0.95f, 0.97f, 1.00f));
        SetColor(theme, ThemeColor::TextMuted, ImVec4(0.57f, 0.64f, 0.70f, 1.00f));
        SetColor(theme, ThemeColor::Surface, ImVec4(0.055f, 0.063f, 0.074f, 0.94f));
        SetColor(theme, ThemeColor::SurfaceRaised, ImVec4(0.105f, 0.122f, 0.141f, 0.96f));
        SetColor(theme, ThemeColor::Border, ImVec4(0.24f, 0.29f, 0.33f, 0.82f));
        SetColor(theme, ThemeColor::Accent, ImVec4(0.18f, 0.62f, 0.92f, 1.00f));
        SetColor(theme, ThemeColor::Success, ImVec4(0.20f, 0.78f, 0.45f, 1.00f));
        SetColor(theme, ThemeColor::Warning, ImVec4(0.95f, 0.65f, 0.23f, 1.00f));
        SetColor(theme, ThemeColor::Danger, ImVec4(0.92f, 0.30f, 0.32f, 1.00f));
        SetColor(theme, ThemeColor::Disabled, ImVec4(0.34f, 0.38f, 0.42f, 1.00f));
        return theme;
    }

    OverlayThemePreset MakeTacticalDarkPreset()
    {
        OverlayThemePreset preset;
        preset.name = "Tactical Dark";
        preset.theme = MakeTacticalDarkTheme();
        return preset;
    }

    void ApplyTheme(const OverlayTheme& theme)
    {
        g_theme = theme;

        if (ImGui::GetCurrentContext() == nullptr)
            return;

        ImGuiStyle& style = ImGui::GetStyle();
        style.Alpha = std::clamp(theme.windowAlpha, 0.20f, 1.0f);
        style.WindowRounding = theme.windowRounding;
        style.ChildRounding = theme.frameRounding;
        style.PopupRounding = theme.frameRounding;
        style.FrameRounding = theme.frameRounding;
        style.GrabRounding = theme.frameRounding;
        style.TabRounding = theme.frameRounding;
        style.ScrollbarRounding = theme.frameRounding;
        style.WindowBorderSize = theme.borderSize;
        style.FrameBorderSize = theme.borderSize;
        style.WindowPadding = theme.windowPadding;
        style.ItemSpacing = theme.itemSpacing;
        style.FramePadding = ImVec2(8.0f, 5.0f);
        style.CellPadding = ImVec2(8.0f, 4.0f);

        ImVec4* colors = style.Colors;
        const ImVec4 text = ColorVec4(ThemeColor::Text);
        const ImVec4 muted = ColorVec4(ThemeColor::TextMuted);
        const ImVec4 surface = ColorVec4(ThemeColor::Surface);
        const ImVec4 raised = ColorVec4(ThemeColor::SurfaceRaised);
        const ImVec4 border = ColorVec4(ThemeColor::Border);
        const ImVec4 accent = ColorVec4(ThemeColor::Accent);
        const ImVec4 success = ColorVec4(ThemeColor::Success);
        const ImVec4 danger = ColorVec4(ThemeColor::Danger);
        const ImVec4 disabled = ColorVec4(ThemeColor::Disabled);

        colors[ImGuiCol_Text] = text;
        colors[ImGuiCol_TextDisabled] = muted;
        colors[ImGuiCol_WindowBg] = surface;
        colors[ImGuiCol_ChildBg] = WithAlpha(surface, 0.82f);
        colors[ImGuiCol_PopupBg] = raised;
        colors[ImGuiCol_Border] = border;
        colors[ImGuiCol_BorderShadow] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
        colors[ImGuiCol_FrameBg] = raised;
        colors[ImGuiCol_FrameBgHovered] = LerpColor(raised, accent, 0.25f);
        colors[ImGuiCol_FrameBgActive] = LerpColor(raised, accent, 0.42f);
        colors[ImGuiCol_TitleBg] = surface;
        colors[ImGuiCol_TitleBgActive] = raised;
        colors[ImGuiCol_TitleBgCollapsed] = WithAlpha(surface, 0.65f);
        colors[ImGuiCol_MenuBarBg] = raised;
        colors[ImGuiCol_ScrollbarBg] = WithAlpha(surface, 0.35f);
        colors[ImGuiCol_ScrollbarGrab] = WithAlpha(disabled, 0.80f);
        colors[ImGuiCol_ScrollbarGrabHovered] = WithAlpha(accent, 0.65f);
        colors[ImGuiCol_ScrollbarGrabActive] = accent;
        colors[ImGuiCol_CheckMark] = success;
        colors[ImGuiCol_SliderGrab] = accent;
        colors[ImGuiCol_SliderGrabActive] = LerpColor(accent, text, 0.18f);
        colors[ImGuiCol_Button] = LerpColor(raised, accent, 0.18f);
        colors[ImGuiCol_ButtonHovered] = LerpColor(raised, accent, 0.34f);
        colors[ImGuiCol_ButtonActive] = LerpColor(raised, accent, 0.50f);
        colors[ImGuiCol_Header] = LerpColor(raised, accent, 0.16f);
        colors[ImGuiCol_HeaderHovered] = LerpColor(raised, accent, 0.30f);
        colors[ImGuiCol_HeaderActive] = LerpColor(raised, accent, 0.45f);
        colors[ImGuiCol_Separator] = border;
        colors[ImGuiCol_SeparatorHovered] = accent;
        colors[ImGuiCol_SeparatorActive] = accent;
        colors[ImGuiCol_ResizeGrip] = WithAlpha(accent, 0.35f);
        colors[ImGuiCol_ResizeGripHovered] = WithAlpha(accent, 0.65f);
        colors[ImGuiCol_ResizeGripActive] = accent;
        colors[ImGuiCol_Tab] = raised;
        colors[ImGuiCol_TabHovered] = LerpColor(raised, accent, 0.35f);
        colors[ImGuiCol_TabSelected] = LerpColor(raised, accent, 0.25f);
        colors[ImGuiCol_TabDimmed] = WithAlpha(raised, 0.65f);
        colors[ImGuiCol_TabDimmedSelected] = LerpColor(raised, accent, 0.12f);
        colors[ImGuiCol_TextSelectedBg] = WithAlpha(accent, 0.35f);
        colors[ImGuiCol_NavHighlight] = accent;
        colors[ImGuiCol_PlotHistogram] = accent;
        colors[ImGuiCol_PlotHistogramHovered] = danger;
    }

    void ApplyThemePreset(const OverlayThemePreset& preset)
    {
        ApplyTheme(preset.theme);
    }

    const OverlayTheme& GetTheme()
    {
        return g_theme;
    }

    OverlayTheme& GetMutableTheme()
    {
        return g_theme;
    }

    void SetTheme(const OverlayTheme& theme)
    {
        ApplyTheme(theme);
    }

    void ResetTheme()
    {
        ApplyTheme(MakeTacticalDarkTheme());
    }

    ImU32 ColorU32(ThemeColor color, float alphaScale)
    {
        return ImGui::ColorConvertFloat4ToU32(ColorVec4(color, alphaScale));
    }

    ImVec4 ColorVec4(ThemeColor color, float alphaScale)
    {
        const int index = std::clamp(ThemeColorIndex(color), 0, ThemeColorIndex(ThemeColor::Count) - 1);
        ImVec4 colorValue = g_theme.colors[index];
        colorValue.w = std::clamp(colorValue.w * alphaScale, 0.0f, 1.0f);
        return colorValue;
    }

    bool DrawThemeEditor(const char* id)
    {
        if (ImGui::GetCurrentContext() == nullptr)
            return false;

        bool changed = false;
        ImGui::PushID(id != nullptr ? id : "OverlayThemeEditor");

        static constexpr const char* kColorNames[] = {
            "Text",
            "Text muted",
            "Surface",
            "Surface raised",
            "Border",
            "Accent",
            "Success",
            "Warning",
            "Danger",
            "Disabled"
        };

        if (ImGui::BeginTable("ThemeColors", 2, ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_RowBg))
        {
            ImGui::TableSetupColumn("Token", ImGuiTableColumnFlags_WidthFixed, 120.0f);
            ImGui::TableSetupColumn("Color");
            for (int index = 0; index < ThemeColorIndex(ThemeColor::Count); ++index)
            {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextUnformatted(kColorNames[index]);
                ImGui::TableSetColumnIndex(1);
                changed |= ImGui::ColorEdit4(kColorNames[index], &g_theme.colors[index].x, ImGuiColorEditFlags_NoInputs);
            }
            ImGui::EndTable();
        }

        changed |= ImGui::SliderFloat("Window alpha", &g_theme.windowAlpha, 0.20f, 1.0f, "%.2f");
        changed |= ImGui::SliderFloat("Window rounding", &g_theme.windowRounding, 0.0f, 16.0f, "%.1f");
        changed |= ImGui::SliderFloat("Frame rounding", &g_theme.frameRounding, 0.0f, 16.0f, "%.1f");
        changed |= ImGui::SliderFloat("Chip rounding", &g_theme.chipRounding, 0.0f, 16.0f, "%.1f");
        changed |= ImGui::SliderFloat("Meter rounding", &g_theme.meterRounding, 0.0f, 16.0f, "%.1f");
        changed |= ImGui::SliderFloat("Border size", &g_theme.borderSize, 0.0f, 3.0f, "%.1f");
        changed |= ImGui::SliderFloat("Row height", &g_theme.rowHeight, 20.0f, 44.0f, "%.1f");
        changed |= ImGui::SliderFloat("Meter height", &g_theme.meterHeight, 8.0f, 30.0f, "%.1f");
        changed |= ImGui::SliderFloat("Sidebar width", &g_theme.sidebarWidth, 96.0f, 280.0f, "%.1f");

        if (ImGui::Button("Reset theme"))
        {
            ResetTheme();
            changed = true;
        }

        if (changed)
            ApplyTheme(g_theme);

        ImGui::PopID();
        return changed;
    }
}
