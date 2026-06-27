#include "Ui/OverlayFonts.h"

#include "Core/OverlayLog.h"
#include "imgui.h"

#include <algorithm>
#include <filesystem>

namespace UniversalOverlay::Ui
{
    namespace
    {
        FontStatus g_status;
        FontConfig g_loadedConfig;
        ImGuiContext* g_loadedContext = nullptr;
        bool g_hasLoadedConfig = false;
        std::string g_lastLoggedError;

        bool SameConfig(const FontConfig& lhs, const FontConfig& rhs)
        {
            return lhs.uiFontPath == rhs.uiFontPath &&
                lhs.iconFontPath == rhs.iconFontPath &&
                lhs.baseSize == rhs.baseSize &&
                lhs.mergeIcons == rhs.mergeIcons;
        }

        void SetFontError(const std::string& message)
        {
            g_status.lastError = message;
            if (g_lastLoggedError == message)
                return;

            g_lastLoggedError = message;
            UniversalOverlay::Log::Warn(UniversalOverlay::Log::LogCategory::Fonts, message.c_str());
        }

        bool FileExists(const std::string& path)
        {
            if (path.empty())
                return false;

            std::error_code error;
            return std::filesystem::exists(path, error) && !error;
        }
    }

    bool LoadFonts(const FontConfig& config)
    {
        ImGuiContext* context = ImGui::GetCurrentContext();
        if (g_hasLoadedConfig && g_loadedContext == context && SameConfig(g_loadedConfig, config))
            return g_status.loadedUiFont && (config.iconFontPath.empty() || g_status.loadedIconFont);

        g_status = FontStatus{};
        g_loadedConfig = config;
        g_loadedContext = context;
        g_hasLoadedConfig = true;

        if (context == nullptr)
        {
            SetFontError("font loading skipped because no ImGui context is active");
            return false;
        }

        ImGuiIO& io = ImGui::GetIO();
        if (io.Fonts->IsBuilt())
        {
            SetFontError("font loading skipped because the ImGui font atlas is already built");
            return false;
        }

        io.Fonts->ClearFonts();
        const float baseSize = std::clamp(config.baseSize, 8.0f, 48.0f);

        ImFont* uiFont = nullptr;
        if (config.uiFontPath.empty())
        {
            ImFontConfig fontConfig;
            fontConfig.SizePixels = baseSize;
            uiFont = io.Fonts->AddFontDefault(&fontConfig);
            g_status.activeUiFont = "ImGui default";
        }
        else if (FileExists(config.uiFontPath))
        {
            ImFontConfig fontConfig;
            fontConfig.SizePixels = baseSize;
            fontConfig.Flags |= ImFontFlags_NoLoadError;
            uiFont = io.Fonts->AddFontFromFileTTF(config.uiFontPath.c_str(), baseSize, &fontConfig);
            g_status.activeUiFont = config.uiFontPath;
        }
        else
        {
            SetFontError("UI font file does not exist");
        }

        g_status.loadedUiFont = uiFont != nullptr;
        if (uiFont != nullptr)
            io.FontDefault = uiFont;

        if (!config.iconFontPath.empty())
        {
            if (FileExists(config.iconFontPath))
            {
                ImFontConfig iconConfig;
                iconConfig.SizePixels = baseSize;
                iconConfig.MergeMode = config.mergeIcons && uiFont != nullptr;
                iconConfig.PixelSnapH = true;
                iconConfig.Flags |= ImFontFlags_NoLoadError;
                ImFont* iconFont = io.Fonts->AddFontFromFileTTF(config.iconFontPath.c_str(), baseSize, &iconConfig);
                g_status.loadedIconFont = iconFont != nullptr;
                if (iconFont != nullptr)
                    g_status.activeIconFont = config.iconFontPath;
                else if (g_status.lastError.empty())
                    SetFontError("icon font could not be loaded");
            }
            else if (g_status.lastError.empty())
            {
                SetFontError("icon font file does not exist");
            }
        }

        if (!g_status.loadedUiFont && g_status.lastError.empty())
            SetFontError("UI font could not be loaded");

        return g_status.loadedUiFont && (config.iconFontPath.empty() || g_status.loadedIconFont);
    }

    const FontStatus& GetFontStatus()
    {
        return g_status;
    }
}
