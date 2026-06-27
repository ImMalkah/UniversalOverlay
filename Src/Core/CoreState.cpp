#include "Core/CoreState.h"

namespace UniversalOverlay
{
    namespace State
    {
        GraphicsAPI api = GraphicsAPI::OpenGL3;
        std::atomic_bool initialized = false;
        bool menuOpen = false;
        HWND windowHandle = nullptr;
        
        int menuToggleKey = VK_OEM_PERIOD;
        int unloadKey = VK_F1;
        bool waitingForMenuToggleKey = false;
        bool waitingForUnloadKey = false;
        bool keyCaptureWaitingForRelease = false;

        bool configLoaded = false;
        bool configDirty = false;
        std::atomic_bool shouldUnload = false;
        std::string settingsWarning = "";
        std::atomic<int> hookRefCount = 0;
        std::function<void()> renderCallback = nullptr;
    }
}
