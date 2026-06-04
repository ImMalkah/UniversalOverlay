#pragma once

#include "UniversalOverlay.h"

namespace UniversalOverlay
{
    namespace Hooks
    {
        bool Install(GraphicsAPI api);
        void Remove();
        void HandleMenuHotkey();
    }
}
