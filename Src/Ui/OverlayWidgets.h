#pragma once

#include "Ui/OverlayTheme.h"
#include "imgui.h"

#include <string>

namespace UniversalOverlay::Ui
{
    enum class WidgetTone
    {
        Neutral,
        Accent,
        Success,
        Warning,
        Danger,
        Disabled
    };

    struct PillSpec
    {
        std::string label;
        WidgetTone tone = WidgetTone::Neutral;
        bool filled = true;
        std::string id;
    };

    struct MeterSpec
    {
        std::string label;
        float value = 0.0f;
        float maxValue = 1.0f;
        WidgetTone tone = WidgetTone::Accent;
        bool disabled = false;
        ImVec2 size = ImVec2(-1.0f, 0.0f);
        std::string id;
    };

    struct SegmentedMeterSpec
    {
        int filledSegments = 0;
        int totalSegments = 1;
        WidgetTone tone = WidgetTone::Accent;
        ImVec2 size = ImVec2(-1.0f, 10.0f);
        std::string id;
    };

    bool StatusPill(const PillSpec& spec);
    bool MetricPill(const char* label, const char* value, WidgetTone tone = WidgetTone::Neutral);
    bool Badge(const PillSpec& spec);
    bool Meter(const MeterSpec& spec);
    bool SegmentedMeter(const SegmentedMeterSpec& spec);
}
