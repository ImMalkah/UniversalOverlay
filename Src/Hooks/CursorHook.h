#pragma once

#include <Windows.h>

namespace UniversalOverlay
{
    void SetMenuOpenForCursor(bool open);
    bool IsMenuOpenForCursor();
    void SetCursorLockWindow(HWND hwnd);
    bool ShouldLockCursorToWindow(HWND hwnd);

    void ShowGameCursor();
    void HideGameCursor();
    void LockCursorToWindow(HWND hwnd);
    void UnlockCursor();
    void ApplyCursorState(HWND hwnd, bool menuOpen);

    bool InstallCursorHooks();
    void RemoveCursorHooks();
}
