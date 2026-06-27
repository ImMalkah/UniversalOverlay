#include "Ui/OverlayGallery.h"

#include "Renderer/RendererDiagnostics.h"
#include "Ui/OverlayFonts.h"
#include "Ui/OverlayTheme.h"
#include "Ui/OverlayWidgets.h"
#include "Ui/OverlayWindowManager.h"
#include "imgui.h"

namespace UniversalOverlay::Ui
{
    namespace
    {
        PillSpec MakePill(const char* id, const char* label, WidgetTone tone)
        {
            PillSpec spec;
            spec.id = id;
            spec.label = label;
            spec.tone = tone;
            spec.filled = true;
            return spec;
        }

        MeterSpec MakeMeter(const char* id, const char* label, float value, float maxValue, WidgetTone tone)
        {
            MeterSpec spec;
            spec.id = id;
            spec.label = label;
            spec.value = value;
            spec.maxValue = maxValue;
            spec.tone = tone;
            return spec;
        }

        SegmentedMeterSpec MakeSegmentedMeter(const char* id, int filledSegments, int totalSegments, WidgetTone tone)
        {
            SegmentedMeterSpec spec;
            spec.id = id;
            spec.filledSegments = filledSegments;
            spec.totalSegments = totalSegments;
            spec.tone = tone;
            return spec;
        }
    }

    void DrawGallery()
    {
        ImGui::SeparatorText("Theme editor");
        DrawThemeEditor("OverlayThemeEditor");

        ImGui::SeparatorText("Status pills");
        StatusPill(MakePill("pill-neutral", "Neutral", WidgetTone::Neutral));
        ImGui::SameLine();
        StatusPill(MakePill("pill-ready", "Ready", WidgetTone::Success));
        ImGui::SameLine();
        StatusPill(MakePill("pill-warning", "Warning", WidgetTone::Warning));
        ImGui::SameLine();
        StatusPill(MakePill("pill-danger", "Danger", WidgetTone::Danger));

        ImGui::SeparatorText("Meters");
        Meter(MakeMeter("meter-empty", "Empty", 0.0f, 100.0f, WidgetTone::Accent));
        Meter(MakeMeter("meter-partial", "Partial", 45.0f, 100.0f, WidgetTone::Warning));
        Meter(MakeMeter("meter-ready", "Ready", 100.0f, 100.0f, WidgetTone::Success));

        ImGui::SeparatorText("Segmented strips");
        SegmentedMeter(MakeSegmentedMeter("segments-empty", 0, 4, WidgetTone::Disabled));
        SegmentedMeter(MakeSegmentedMeter("segments-partial", 2, 4, WidgetTone::Warning));
        SegmentedMeter(MakeSegmentedMeter("segments-ready", 4, 4, WidgetTone::Success));

        ImGui::SeparatorText("Managed windows");
        for (const ManagedWindowState& state : GetWindowStates())
        {
            ImGui::Text("%s open=%s pinned=%s input=%s",
                state.name.c_str(),
                state.open ? "true" : "false",
                state.pinned ? "true" : "false",
                state.acceptingInput ? "true" : "false");
        }

        ImGui::SeparatorText("Diagnostics");
        const FontStatus& font = GetFontStatus();
        ImGui::Text("UI font loaded: %s", font.loadedUiFont ? "true" : "false");
        ImGui::Text("UI font: %s", font.activeUiFont.empty() ? "default" : font.activeUiFont.c_str());
        ImGui::Text("Icon font loaded: %s", font.loadedIconFont ? "true" : "false");
        ImGui::Text("Icon font: %s", font.activeIconFont.empty() ? "none" : font.activeIconFont.c_str());
        if (!font.lastError.empty())
            ImGui::Text("Font status: %s", font.lastError.c_str());

        const auto& renderer = Renderer::GetDiagnostics();
        ImGui::Text("Renderer initialized: %s", renderer.initialized ? "true" : "false");
        ImGui::Text("Renderer backend: %s", renderer.activeBackendName.c_str());
        if (!renderer.lastError.empty())
            ImGui::Text("Renderer status: %s", renderer.lastError.c_str());
    }
}
