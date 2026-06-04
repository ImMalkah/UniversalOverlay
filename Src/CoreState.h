#pragma once

#include "UniversalOverlay.h"
#include <atomic>

namespace UniversalOverlay
{
    namespace State
    {
        extern GraphicsAPI api;
        extern std::atomic_bool initialized;
        extern bool menuOpen;
        extern HWND windowHandle;
        
        // Keybind State
        extern int menuToggleKey;
        extern int unloadKey;
        extern bool waitingForMenuToggleKey;
        extern bool waitingForUnloadKey;
        extern bool keyCaptureWaitingForRelease;

        // Config Flags
        extern bool configLoaded;
        extern bool configDirty;
        extern std::atomic_bool shouldUnload;
        extern std::string settingsWarning;
        extern std::atomic<int> hookRefCount;
        extern std::function<void()> renderCallback;
    }
}
