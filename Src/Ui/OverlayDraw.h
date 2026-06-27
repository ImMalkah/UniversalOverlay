#pragma once

#include "imgui.h"

namespace UniversalOverlay::Ui
{
    void DrawGradientRect(ImDrawList* drawList, const ImVec2& min, const ImVec2& max, ImU32 startColor, ImU32 endColor, bool horizontal = true);
    void DrawGradientRect(const ImVec2& min, const ImVec2& max, ImU32 startColor, ImU32 endColor, bool horizontal = true);
    void DrawSeparator(ImDrawList* drawList, const ImVec2& start, float width, ImU32 color, float thickness = 1.0f);
    void DrawSeparator(const ImVec2& start, float width, ImU32 color, float thickness = 1.0f);
    void DrawGlowBar(ImDrawList* drawList, const ImVec2& min, const ImVec2& max, ImU32 color, float glowRadius = 6.0f);
    void DrawGlowBar(const ImVec2& min, const ImVec2& max, ImU32 color, float glowRadius = 6.0f);
    void DrawMeterFill(ImDrawList* drawList, const ImVec2& min, const ImVec2& max, float fraction, ImU32 fillColor, ImU32 backgroundColor, float rounding = 0.0f);
    void DrawMeterFill(const ImVec2& min, const ImVec2& max, float fraction, ImU32 fillColor, ImU32 backgroundColor, float rounding = 0.0f);
}
