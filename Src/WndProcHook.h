#pragma once

#include <Windows.h>

namespace UniversalOverlay
{
    void SetMenuOpenForWndProc(bool open);
    bool InstallWndProcHook(HWND hwnd);
    void RemoveWndProcHook();
}
