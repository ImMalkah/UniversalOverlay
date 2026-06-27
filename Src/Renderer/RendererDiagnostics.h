#pragma once

#include "Renderer/RendererTypes.h"

#include <string>

namespace UniversalOverlay::Renderer
{
    struct RendererDiagnostics
    {
        bool initialized = false;
        GraphicsAPI activeBackend = GraphicsAPI::OpenGL3;
        std::string activeBackendName;
        std::string lastError;
    };

    const RendererDiagnostics& GetDiagnostics();
}
