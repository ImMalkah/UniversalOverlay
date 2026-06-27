#pragma once

#include "imgui.h"

#include <array>
#include <string>

namespace UniversalOverlay::Ui
{
    enum class ThemeColor
    {
        Text,
        TextMuted,
        Surface,
        SurfaceRaised,
        Border,
        Accent,
        Success,
        Warning,
        Danger,
        Disabled,
        Count
    };

    struct OverlayTheme
    {
        std::array<ImVec4, static_cast<int>(ThemeColor::Count)> colors = {};
        float windowRounding = 6.0f;
        float frameRounding = 4.0f;
        float chipRounding = 5.0f;
        float meterRounding = 4.0f;
        float borderSize = 1.0f;
        float windowAlpha = 0.94f;
        ImVec2 windowPadding = ImVec2(12.0f, 10.0f);
        ImVec2 itemSpacing = ImVec2(8.0f, 6.0f);
        float rowHeight = 28.0f;
        float meterHeight = 18.0f;
        float chipHeight = 24.0f;
        float sidebarWidth = 156.0f;
    };

    struct OverlayThemePreset
    {
        const char* name = "Tactical Dark";
        OverlayTheme theme;
    };

    OverlayTheme MakeTacticalDarkTheme();
    OverlayThemePreset MakeTacticalDarkPreset();
    void ApplyTheme(const OverlayTheme& theme);
    void ApplyThemePreset(const OverlayThemePreset& preset);
    const OverlayTheme& GetTheme();
    OverlayTheme& GetMutableTheme();
    void SetTheme(const OverlayTheme& theme);
    void ResetTheme();
    ImU32 ColorU32(ThemeColor color, float alphaScale = 1.0f);
    ImVec4 ColorVec4(ThemeColor color, float alphaScale = 1.0f);
    bool DrawThemeEditor(const char* id);
}
