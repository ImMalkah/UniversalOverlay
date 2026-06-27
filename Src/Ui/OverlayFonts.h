#pragma once

#include <string>

namespace UniversalOverlay::Ui
{
    struct FontConfig
    {
        std::string uiFontPath;
        std::string iconFontPath;
        float baseSize = 16.0f;
        bool mergeIcons = true;
    };

    struct FontStatus
    {
        bool loadedUiFont = false;
        bool loadedIconFont = false;
        std::string activeUiFont;
        std::string activeIconFont;
        std::string lastError;
    };

    bool LoadFonts(const FontConfig& config);
    const FontStatus& GetFontStatus();
}
