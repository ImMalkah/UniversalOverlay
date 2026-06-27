#include "Hooks/WndProcHook.h"
#include "Core/CoreState.h"
#include "Core/Log.h"
#include "UniversalOverlay.h"
#include "imgui.h"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(
    HWND hWnd,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam
);

namespace UniversalOverlay
{
    static HWND g_hwnd = nullptr;
    static WNDPROC g_originalWndProc = nullptr;
    static bool g_menuOpenForWndProc = true;

    static bool IsKeyDownMessage(UINT msg)
    {
        return msg == WM_KEYDOWN || msg == WM_SYSKEYDOWN;
    }

    static void HandleUnloadHotkeyMessage(HWND, UINT msg, WPARAM wParam, LPARAM)
    {
        if (!IsKeyDownMessage(msg))
            return;

        if (State::waitingForMenuToggleKey || State::waitingForUnloadKey)
            return;

        const int unloadKey = State::unloadKey & 0xFFFF;
        if (unloadKey == 0 || static_cast<int>(wParam) != unloadKey)
            return;

        if (!IsHotkeyComboPressed(State::unloadKey))
            return;

        if (!State::shouldUnload.load())
            Log::Debug("Unload hotkey detected from WndProc.");

        RequestUnload();
        g_menuOpenForWndProc = false;
    }

    void SetMenuOpenForWndProc(bool open)
    {
        g_menuOpenForWndProc = open;
    }

    LRESULT CALLBACK hkWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        HandleUnloadHotkeyMessage(hwnd, msg, wParam, lParam);

        if (g_menuOpenForWndProc && ImGui::GetCurrentContext())
        {
            ImGui_ImplWin32_WndProcHandler(hwnd, msg, wParam, lParam);

            ImGuiIO& io = ImGui::GetIO();

            // If ImGui is capturing input, do not pass to game
            if (io.WantCaptureMouse || io.WantCaptureKeyboard)
                return TRUE;
        }

        if (g_originalWndProc)
            return CallWindowProc(g_originalWndProc, hwnd, msg, wParam, lParam);

        return DefWindowProc(hwnd, msg, wParam, lParam);
    }

    bool InstallWndProcHook(HWND hwnd)
    {
        if (!hwnd || g_originalWndProc)
            return false;

        g_hwnd = hwnd;

        g_originalWndProc = reinterpret_cast<WNDPROC>(
            SetWindowLongPtr(
                g_hwnd,
                GWLP_WNDPROC,
                reinterpret_cast<LONG_PTR>(hkWndProc)
            )
        );

        Log::Debug("WndProc hook installed, original WndProc: 0x%p", g_originalWndProc);
        return g_originalWndProc != nullptr;
    }

    void RemoveWndProcHook()
    {
        if (g_hwnd && g_originalWndProc)
        {
            SetWindowLongPtr(
                g_hwnd,
                GWLP_WNDPROC,
                reinterpret_cast<LONG_PTR>(g_originalWndProc)
            );
        }

        g_hwnd = nullptr;
        g_originalWndProc = nullptr;
        Log::Debug("WndProc hook removed");
    }
}
