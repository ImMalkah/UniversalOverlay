#include "Ui/OverlayWidgets.h"

#include "Core/OverlayLog.h"
#include "Ui/OverlayDraw.h"

#include <algorithm>
#include <cmath>

namespace UniversalOverlay::Ui
{
    namespace
    {
        bool g_loggedInvalidMeter = false;
        bool g_loggedInvalidSegmentedMeter = false;
        constexpr int kMaxSegments = 64;

        const char* ResolveWidgetId(const std::string& id, const std::string& label, const char* fallback)
        {
            if (!id.empty())
                return id.c_str();
            if (!label.empty())
                return label.c_str();
            return fallback;
        }

        ThemeColor ToneColor(WidgetTone tone)
        {
            switch (tone)
            {
            case WidgetTone::Accent:
                return ThemeColor::Accent;
            case WidgetTone::Success:
                return ThemeColor::Success;
            case WidgetTone::Warning:
                return ThemeColor::Warning;
            case WidgetTone::Danger:
                return ThemeColor::Danger;
            case WidgetTone::Disabled:
                return ThemeColor::Disabled;
            case WidgetTone::Neutral:
            default:
                return ThemeColor::SurfaceRaised;
            }
        }

        ImU32 TextColorForTone(WidgetTone tone, bool filled)
        {
            if (tone == WidgetTone::Disabled)
                return ColorU32(ThemeColor::TextMuted);

            return filled ? ColorU32(ThemeColor::Text) : ColorU32(ToneColor(tone));
        }

        ImVec2 ResolveItemSize(ImVec2 requested, float fallbackHeight)
        {
            const float width = requested.x < 0.0f ? ImGui::GetContentRegionAvail().x : requested.x;
            const float height = requested.y <= 0.0f ? fallbackHeight : requested.y;
            return ImVec2(std::max(width, 1.0f), std::max(height, 1.0f));
        }

        bool DrawPill(const PillSpec& spec, bool badge)
        {
            const OverlayTheme& theme = GetTheme();
            const char* label = spec.label.c_str();
            const ImVec2 textSize = ImGui::CalcTextSize(label);
            const float horizontalPadding = badge ? 8.0f : 10.0f;
            const ImVec2 size(
                std::max(textSize.x + (horizontalPadding * 2.0f), theme.chipHeight),
                theme.chipHeight);

            ImGui::PushID(ResolveWidgetId(spec.id, spec.label, badge ? "Badge" : "StatusPill"));
            const bool clicked = ImGui::InvisibleButton(badge ? "##badge" : "##pill", size);
            const bool hovered = ImGui::IsItemHovered();
            const bool active = ImGui::IsItemActive();
            const ImVec2 min = ImGui::GetItemRectMin();
            const ImVec2 max = ImGui::GetItemRectMax();
            ImDrawList* drawList = ImGui::GetWindowDrawList();

            const ThemeColor toneColor = ToneColor(spec.tone);
            const float alpha = active ? 1.0f : (hovered ? 0.88f : 0.72f);
            const ImU32 fill = spec.filled ? ColorU32(toneColor, alpha) : ColorU32(ThemeColor::Surface, 0.35f);
            const ImU32 border = ColorU32(toneColor, hovered ? 1.0f : 0.62f);
            drawList->AddRectFilled(min, max, fill, theme.chipRounding);
            drawList->AddRect(min, max, border, theme.chipRounding, 0, theme.borderSize);

            const ImVec2 textPos(
                min.x + ((size.x - textSize.x) * 0.5f),
                min.y + ((size.y - textSize.y) * 0.5f));
            drawList->AddText(textPos, TextColorForTone(spec.tone, spec.filled), label);
            ImGui::PopID();
            return clicked;
        }

        void LogInvalidMeterOnce()
        {
            if (g_loggedInvalidMeter)
                return;

            g_loggedInvalidMeter = true;
            UniversalOverlay::Log::Warn(UniversalOverlay::Log::LogCategory::Ui, "meter widget received a non-positive max value");
        }

        void LogInvalidSegmentedMeterOnce()
        {
            if (g_loggedInvalidSegmentedMeter)
                return;

            g_loggedInvalidSegmentedMeter = true;
            UniversalOverlay::Log::Warn(UniversalOverlay::Log::LogCategory::Ui, "segmented meter widget received a non-positive segment count");
        }
    }

    bool StatusPill(const PillSpec& spec)
    {
        return DrawPill(spec, false);
    }

    bool MetricPill(const char* label, const char* value, WidgetTone tone)
    {
        std::string text;
        if (label != nullptr && label[0] != '\0')
        {
            text += label;
            text += ": ";
        }
        text += value != nullptr ? value : "";

        PillSpec spec;
        spec.label = text;
        spec.tone = tone;
        spec.filled = true;
        return StatusPill(spec);
    }

    bool Badge(const PillSpec& spec)
    {
        return DrawPill(spec, true);
    }

    bool Meter(const MeterSpec& spec)
    {
        const OverlayTheme& theme = GetTheme();
        if (!std::isfinite(spec.maxValue) || spec.maxValue <= 0.0f)
            LogInvalidMeterOnce();

        const float maxValue = (std::isfinite(spec.maxValue) && spec.maxValue > 0.0f) ? spec.maxValue : 1.0f;
        const float value = std::isfinite(spec.value) ? spec.value : 0.0f;
        const float fraction = std::clamp(value / maxValue, 0.0f, 1.0f);
        const ImVec2 size = ResolveItemSize(spec.size, theme.meterHeight);
        ImGui::PushID(ResolveWidgetId(spec.id, spec.label, "Meter"));
        ImGui::InvisibleButton("##meter", size);
        const bool hovered = ImGui::IsItemHovered();
        const ImVec2 min = ImGui::GetItemRectMin();
        const ImVec2 max = ImGui::GetItemRectMax();

        const WidgetTone effectiveTone = spec.disabled ? WidgetTone::Disabled : spec.tone;
        const ImU32 background = ColorU32(ThemeColor::SurfaceRaised, hovered ? 0.82f : 0.62f);
        const ImU32 fill = ColorU32(ToneColor(effectiveTone), spec.disabled ? 0.42f : 0.92f);
        DrawMeterFill(ImGui::GetWindowDrawList(), min, max, fraction, fill, background, theme.meterRounding);
        ImGui::GetWindowDrawList()->AddRect(min, max, ColorU32(ThemeColor::Border, hovered ? 1.0f : 0.70f), theme.meterRounding, 0, theme.borderSize);

        if (!spec.label.empty())
        {
            const ImVec2 textSize = ImGui::CalcTextSize(spec.label.c_str());
            const ImVec2 textPos(min.x + 8.0f, min.y + ((size.y - textSize.y) * 0.5f));
            ImGui::GetWindowDrawList()->AddText(textPos, ColorU32(spec.disabled ? ThemeColor::TextMuted : ThemeColor::Text), spec.label.c_str());
        }

        ImGui::PopID();
        return hovered;
    }

    bool SegmentedMeter(const SegmentedMeterSpec& spec)
    {
        if (spec.totalSegments <= 0)
            LogInvalidSegmentedMeterOnce();

        const int totalSegments = std::clamp(spec.totalSegments, 1, kMaxSegments);
        const int filledSegments = std::clamp(spec.filledSegments, 0, totalSegments);
        const ImVec2 size = ResolveItemSize(spec.size, spec.size.y > 0.0f ? spec.size.y : 10.0f);
        ImGui::PushID(ResolveWidgetId(spec.id, std::string{}, "SegmentedMeter"));
        ImGui::InvisibleButton("##segments", size);
        const bool hovered = ImGui::IsItemHovered();
        const ImVec2 min = ImGui::GetItemRectMin();
        const float gap = totalSegments > 1 ? std::min(3.0f, size.x / static_cast<float>(totalSegments) * 0.25f) : 0.0f;
        const float availableWidth = std::max(size.x - (gap * static_cast<float>(totalSegments - 1)), static_cast<float>(totalSegments));
        const float segmentWidth = availableWidth / static_cast<float>(totalSegments);
        ImDrawList* drawList = ImGui::GetWindowDrawList();

        for (int index = 0; index < totalSegments; ++index)
        {
            const float x = min.x + ((segmentWidth + gap) * static_cast<float>(index));
            const ImVec2 segmentMin(x, min.y);
            const ImVec2 segmentMax(x + std::max(segmentWidth, 1.0f), min.y + size.y);
            const bool filled = index < filledSegments;
            const ImU32 color = filled
                ? ColorU32(ToneColor(spec.tone), hovered ? 1.0f : 0.84f)
                : ColorU32(ThemeColor::SurfaceRaised, 0.52f);
            drawList->AddRectFilled(segmentMin, segmentMax, color, size.y * 0.5f);
        }

        ImGui::PopID();
        return hovered;
    }
}
