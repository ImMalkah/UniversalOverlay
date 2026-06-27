#include "Ui/OverlayDraw.h"

#include <algorithm>

namespace UniversalOverlay::Ui
{
    namespace
    {
        ImDrawList* ResolveDrawList(ImDrawList* drawList)
        {
            if (drawList != nullptr)
                return drawList;

            if (ImGui::GetCurrentContext() == nullptr)
                return nullptr;

            return ImGui::GetWindowDrawList();
        }

        ImU32 WithAlpha(ImU32 color, float alphaScale)
        {
            ImVec4 value = ImGui::ColorConvertU32ToFloat4(color);
            value.w = std::clamp(value.w * alphaScale, 0.0f, 1.0f);
            return ImGui::ColorConvertFloat4ToU32(value);
        }
    }

    void DrawGradientRect(ImDrawList* drawList, const ImVec2& min, const ImVec2& max, ImU32 startColor, ImU32 endColor, bool horizontal)
    {
        drawList = ResolveDrawList(drawList);
        if (drawList == nullptr || max.x <= min.x || max.y <= min.y)
            return;

        if (horizontal)
            drawList->AddRectFilledMultiColor(min, max, startColor, endColor, endColor, startColor);
        else
            drawList->AddRectFilledMultiColor(min, max, startColor, startColor, endColor, endColor);
    }

    void DrawGradientRect(const ImVec2& min, const ImVec2& max, ImU32 startColor, ImU32 endColor, bool horizontal)
    {
        DrawGradientRect(nullptr, min, max, startColor, endColor, horizontal);
    }

    void DrawSeparator(ImDrawList* drawList, const ImVec2& start, float width, ImU32 color, float thickness)
    {
        drawList = ResolveDrawList(drawList);
        if (drawList == nullptr || width <= 0.0f || thickness <= 0.0f)
            return;

        drawList->AddLine(start, ImVec2(start.x + width, start.y), color, thickness);
    }

    void DrawSeparator(const ImVec2& start, float width, ImU32 color, float thickness)
    {
        DrawSeparator(nullptr, start, width, color, thickness);
    }

    void DrawGlowBar(ImDrawList* drawList, const ImVec2& min, const ImVec2& max, ImU32 color, float glowRadius)
    {
        drawList = ResolveDrawList(drawList);
        if (drawList == nullptr || max.x <= min.x || max.y <= min.y)
            return;

        const float radius = std::max(0.0f, glowRadius);
        if (radius > 0.0f)
        {
            drawList->AddRectFilled(
                ImVec2(min.x - radius, min.y - radius),
                ImVec2(max.x + radius, max.y + radius),
                WithAlpha(color, 0.10f),
                radius);
            drawList->AddRectFilled(
                ImVec2(min.x - (radius * 0.5f), min.y - (radius * 0.5f)),
                ImVec2(max.x + (radius * 0.5f), max.y + (radius * 0.5f)),
                WithAlpha(color, 0.18f),
                radius * 0.5f);
        }

        drawList->AddRectFilled(min, max, color, (max.y - min.y) * 0.5f);
    }

    void DrawGlowBar(const ImVec2& min, const ImVec2& max, ImU32 color, float glowRadius)
    {
        DrawGlowBar(nullptr, min, max, color, glowRadius);
    }

    void DrawMeterFill(ImDrawList* drawList, const ImVec2& min, const ImVec2& max, float fraction, ImU32 fillColor, ImU32 backgroundColor, float rounding)
    {
        drawList = ResolveDrawList(drawList);
        if (drawList == nullptr || max.x <= min.x || max.y <= min.y)
            return;

        const float clampedFraction = std::clamp(fraction, 0.0f, 1.0f);
        const float safeRounding = std::max(0.0f, rounding);
        drawList->AddRectFilled(min, max, backgroundColor, safeRounding);

        if (clampedFraction <= 0.0f)
            return;

        const float fillMaxX = min.x + ((max.x - min.x) * clampedFraction);
        drawList->AddRectFilled(min, ImVec2(fillMaxX, max.y), fillColor, safeRounding);
    }

    void DrawMeterFill(const ImVec2& min, const ImVec2& max, float fraction, ImU32 fillColor, ImU32 backgroundColor, float rounding)
    {
        DrawMeterFill(nullptr, min, max, fraction, fillColor, backgroundColor, rounding);
    }
}
