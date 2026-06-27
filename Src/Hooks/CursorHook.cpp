#include "Hooks/CursorHook.h"
#include "Core/Log.h"
#include "MinHook.h"

namespace UniversalOverlay
{
    static bool g_cursorMenuOpen = true;
    static bool g_lastAppliedMenuOpen = false;
    static HWND g_cursorLockHwnd = nullptr;

    using SetCursorPos_t = BOOL(WINAPI*)(int X, int Y);
    using ClipCursor_t = BOOL(WINAPI*)(const RECT* lpRect);

    static SetCursorPos_t oSetCursorPos = nullptr;
    static ClipCursor_t oClipCursor = nullptr;

    void SetMenuOpenForCursor(bool open)
    {
        g_cursorMenuOpen = open;
    }

    bool IsMenuOpenForCursor()
    {
        return g_cursorMenuOpen;
    }

    void SetCursorLockWindow(HWND hwnd)
    {
        g_cursorLockHwnd = hwnd;
    }

    bool ShouldLockCursorToWindow(HWND hwnd)
    {
        if (!hwnd)
            return false;

        if (IsIconic(hwnd))
            return false;

        return GetForegroundWindow() == hwnd;
    }

    static bool ShouldAllowHostCursorControl()
    {
        return !g_cursorMenuOpen && ShouldLockCursorToWindow(g_cursorLockHwnd);
    }

    BOOL WINAPI hkSetCursorPos(int X, int Y)
    {
        if (!ShouldAllowHostCursorControl())
            return TRUE;

        return oSetCursorPos(X, Y);
    }

    BOOL WINAPI hkClipCursor(const RECT* lpRect)
    {
        if (!ShouldAllowHostCursorControl())
            return oClipCursor(nullptr);

        return oClipCursor(lpRect);
    }

    void ShowGameCursor()
    {
        ClipCursor(nullptr);
        ReleaseCapture();

        while (ShowCursor(TRUE) < 0)
        {
        }
    }

    void HideGameCursor()
    {
        while (ShowCursor(FALSE) >= 0)
        {
        }
    }

    void LockCursorToWindow(HWND hwnd)
    {
        if (!ShouldLockCursorToWindow(hwnd))
        {
            UnlockCursor();
            return;
        }

        RECT rect;
        GetClientRect(hwnd, &rect);

        POINT topLeft = { rect.left, rect.top };
        POINT bottomRight = { rect.right, rect.bottom };

        ClientToScreen(hwnd, &topLeft);
        ClientToScreen(hwnd, &bottomRight);

        RECT clipRect;
        clipRect.left = topLeft.x;
        clipRect.top = topLeft.y;
        clipRect.right = bottomRight.x;
        clipRect.bottom = bottomRight.y;

        ClipCursor(&clipRect);
    }

    void UnlockCursor()
    {
        ClipCursor(nullptr);
        ReleaseCapture();
    }

    void ApplyCursorState(HWND hwnd, bool menuOpen)
    {
        SetMenuOpenForCursor(menuOpen);
        SetCursorLockWindow(hwnd);

        if (menuOpen)
        {
            if (g_lastAppliedMenuOpen != menuOpen)
                ShowGameCursor();

            g_lastAppliedMenuOpen = menuOpen;
            UnlockCursor();
            return;
        }

        if (g_lastAppliedMenuOpen != menuOpen)
        {
            g_lastAppliedMenuOpen = menuOpen;
            UnlockCursor();
        }

        if (ShouldLockCursorToWindow(hwnd))
        {
            LockCursorToWindow(hwnd);
            return;
        }

        UnlockCursor();
    }

    bool InstallCursorHooks()
    {
        HMODULE user32 = GetModuleHandleA("user32.dll");
        if (!user32)
            return false;

        void* setCursorPosTarget = GetProcAddress(user32, "SetCursorPos");
        if (!setCursorPosTarget)
            return false;

        void* clipCursorTarget = GetProcAddress(user32, "ClipCursor");
        if (!clipCursorTarget)
            return false;

        if (MH_CreateHook(
            setCursorPosTarget,
            &hkSetCursorPos,
            reinterpret_cast<void**>(&oSetCursorPos)
        ) != MH_OK)
        {
            return false;
        }

        if (MH_CreateHook(
            clipCursorTarget,
            &hkClipCursor,
            reinterpret_cast<void**>(&oClipCursor)
        ) != MH_OK)
        {
            return false;
        }

        Log::Debug("Cursor hooks registered with MinHook");
        return true;
    }

    void RemoveCursorHooks()
    {
        oSetCursorPos = nullptr;
        oClipCursor = nullptr;
    }
}
