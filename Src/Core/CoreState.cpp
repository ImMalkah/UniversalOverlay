#include "Core/CoreState.h"

namespace UniversalOverlay
{
    namespace State
    {
        GraphicsAPI api = GraphicsAPI::OpenGL3;
        std::atomic_bool initialized = false;
        bool menuOpen = false;
        HWND windowHandle = nullptr;
        float menuPositionX = 80.0f;
        float menuPositionY = 80.0f;
        float menuSizeX = 550.0f;
        float menuSizeY = 380.0f;
        bool applySavedMenuPlacement = false;
        
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
