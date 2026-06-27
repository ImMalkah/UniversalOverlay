#include "Menu.h"
#include "CoreState.h"
#include "UIRegistry.h"
#include "ConfigSystem.h"
#include "Log.h"
#include "imgui.h"

#include <cstdio>

namespace UniversalOverlay
{
    // Shared Helper functions declared in UniversalOverlay.h
    int GetPressedKey()
    {
        for (int key = 1; key < 256; key++)
        {
            if (GetAsyncKeyState(key) & 1)
            {
                return key;
            }
        }
        return 0;
    }

    const char* GetKeyName(int key)
    {
        switch (key)
        {
        case VK_LBUTTON: return "Mouse Left";
        case VK_RBUTTON: return "Mouse Right";
        case VK_MBUTTON: return "Mouse Middle";
        case VK_XBUTTON1: return "Mouse X1";
        case VK_XBUTTON2: return "Mouse X2";

        case VK_BACK: return "Backspace";
        case VK_TAB: return "TAB";
        case VK_RETURN: return "ENTER";
        case VK_SHIFT: return "SHIFT";
        case VK_CONTROL: return "CTRL";
        case VK_MENU: return "ALT";
        case VK_PAUSE: return "PAUSE";
        case VK_CAPITAL: return "CAPS LOCK";
        case VK_ESCAPE: return "ESCAPE";
        case VK_SPACE: return "SPACE";

        case VK_PRIOR: return "PAGE UP";
        case VK_NEXT: return "PAGE DOWN";
        case VK_END: return "END";
        case VK_HOME: return "HOME";
        case VK_LEFT: return "LEFT";
        case VK_UP: return "UP";
        case VK_RIGHT: return "RIGHT";
        case VK_DOWN: return "DOWN";
        case VK_INSERT: return "INSERT";
        case VK_DELETE: return "DELETE";
        case VK_OEM_PERIOD: return ".";

        case VK_NUMPAD0: return "NUMPAD 0";
        case VK_NUMPAD1: return "NUMPAD 1";
        case VK_NUMPAD2: return "NUMPAD 2";
        case VK_NUMPAD3: return "NUMPAD 3";
        case VK_NUMPAD4: return "NUMPAD 4";
        case VK_NUMPAD5: return "NUMPAD 5";
        case VK_NUMPAD6: return "NUMPAD 6";
        case VK_NUMPAD7: return "NUMPAD 7";
        case VK_NUMPAD8: return "NUMPAD 8";
        case VK_NUMPAD9: return "NUMPAD 9";

        case VK_MULTIPLY: return "NUMPAD *";
        case VK_ADD: return "NUMPAD +";
        case VK_SUBTRACT: return "NUMPAD -";
        case VK_DECIMAL: return "NUMPAD .";
        case VK_DIVIDE: return "NUMPAD /";

        case VK_F1: return "F1";
        case VK_F2: return "F2";
        case VK_F3: return "F3";
        case VK_F4: return "F4";
        case VK_F5: return "F5";
        case VK_F6: return "F6";
        case VK_F7: return "F7";
        case VK_F8: return "F8";
        case VK_F9: return "F9";
        case VK_F10: return "F10";
        case VK_F11: return "F11";
        case VK_F12: return "F12";

        default:
            break;
        }

        static char name[32];

        if (key >= 'A' && key <= 'Z')
        {
            sprintf_s(name, "%c", key);
            return name;
        }

        if (key >= '0' && key <= '9')
        {
            sprintf_s(name, "%c", key);
            return name;
        }

        sprintf_s(name, "VK_%d", key);
        return name;
    }

    bool IsHotkeyComboPressed(int keybindCode)
    {
        if (keybindCode == 0)
            return false;

        int key = keybindCode & 0xFFFF;
        if (key == 0)
            return false;

        bool reqCtrl = (keybindCode & HOTKEY_MOD_CTRL) != 0;
        bool reqShift = (keybindCode & HOTKEY_MOD_SHIFT) != 0;
        bool reqAlt = (keybindCode & HOTKEY_MOD_ALT) != 0;

        // Check if main key is pressed
        bool keyDown = (GetAsyncKeyState(key) & 0x8000) != 0;
        if (!keyDown)
            return false;

        // Check modifier states
        bool ctrlDown = (GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0;
        bool shiftDown = (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;
        bool altDown = (GetAsyncKeyState(VK_MENU) & 0x8000) != 0;

        return (ctrlDown == reqCtrl) && (shiftDown == reqShift) && (altDown == reqAlt);
    }

    std::string GetHotkeyComboName(int keybindCode)
    {
        if (keybindCode == 0)
            return "None";

        int key = keybindCode & 0xFFFF;
        bool ctrl = (keybindCode & HOTKEY_MOD_CTRL) != 0;
        bool shift = (keybindCode & HOTKEY_MOD_SHIFT) != 0;
        bool alt = (keybindCode & HOTKEY_MOD_ALT) != 0;

        std::string comboName = "";
        if (ctrl) comboName += "Ctrl + ";
        if (shift) comboName += "Shift + ";
        if (alt) comboName += "Alt + ";

        comboName += GetKeyName(key);
        return comboName;
    }

    namespace Menu
    {
        static bool IsAnyMouseButtonDown()
        {
            return
                (GetAsyncKeyState(VK_LBUTTON) & 0x8000) ||
                (GetAsyncKeyState(VK_RBUTTON) & 0x8000) ||
                (GetAsyncKeyState(VK_MBUTTON) & 0x8000) ||
                (GetAsyncKeyState(VK_XBUTTON1) & 0x8000) ||
                (GetAsyncKeyState(VK_XBUTTON2) & 0x8000);
        }

        static void ClearAsyncKeyState()
        {
            for (int key = 1; key < 256; key++)
            {
                GetAsyncKeyState(key);
            }
        }

        static bool IsModifierKey(int key)
        {
            return key == VK_CONTROL || key == VK_LCONTROL || key == VK_RCONTROL ||
                   key == VK_SHIFT || key == VK_LSHIFT || key == VK_RSHIFT ||
                   key == VK_MENU || key == VK_LMENU || key == VK_RMENU;
        }

        static void DrawKeybindControl(const char* label, int& keybindCode, bool& waiting)
        {
            ImGui::Text("%s: %s", label, GetHotkeyComboName(keybindCode).c_str());
            ImGui::SameLine();

            char buttonLabel[128] = {};
            if (waiting)
            {
                sprintf_s(buttonLabel, "Press any key...##%s", label);
            }
            else
            {
                sprintf_s(buttonLabel, "Bind##%s", label);
            }

            if (ImGui::Button(buttonLabel))
            {
                State::waitingForMenuToggleKey = false;
                State::waitingForUnloadKey = false;

                waiting = true;
                State::keyCaptureWaitingForRelease = true;
                ClearAsyncKeyState();
                Log::Debug("Started key capture for control: %s", label);
            }

            ImGui::SameLine();
            if (ImGui::Button(("Clear##" + std::string(label)).c_str()))
            {
                keybindCode = 0;
                State::configDirty = true;
                State::settingsWarning = "";
            }

            if (!waiting)
                return;

            ImGui::SameLine();

            if (State::keyCaptureWaitingForRelease)
            {
                ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.2f, 1.0f), "release mouse...");
                if (!IsAnyMouseButtonDown())
                {
                    ClearAsyncKeyState();
                    State::keyCaptureWaitingForRelease = false;
                    Log::Debug("Key capture armed.");
                }
                return;
            }

            bool ctrlPressed = (GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0;
            bool shiftPressed = (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;
            bool altPressed = (GetAsyncKeyState(VK_MENU) & 0x8000) != 0;

            std::string liveCombo = "";
            if (ctrlPressed) liveCombo += "Ctrl+";
            if (shiftPressed) liveCombo += "Shift+";
            if (altPressed) liveCombo += "Alt+";
            liveCombo += "...";

            ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.2f, 1.0f), "waiting (%s)", liveCombo.c_str());
            
            int pressedKey = GetPressedKey();
            if (pressedKey == 0)
                return;

            if (pressedKey == VK_ESCAPE)
            {
                waiting = false;
                Log::Debug("Key capture cancelled.");
                return;
            }

            if (IsModifierKey(pressedKey))
                return;

            int finalCombo = pressedKey;
            if (ctrlPressed) finalCombo |= HOTKEY_MOD_CTRL;
            if (shiftPressed) finalCombo |= HOTKEY_MOD_SHIFT;
            if (altPressed) finalCombo |= HOTKEY_MOD_ALT;

            // Conflict Resolution
            if (strcmp(label, "Menu Toggle") == 0)
            {
                if (finalCombo != 0 && finalCombo == State::unloadKey)
                {
                    State::unloadKey = 0;
                    State::settingsWarning = "Warning: Unload Module was cleared due to hotkey conflict!";
                }
                else
                {
                    State::settingsWarning = "";
                }
            }
            else if (strcmp(label, "Unload Module") == 0)
            {
                if (finalCombo != 0 && finalCombo == State::menuToggleKey)
                {
                    State::menuToggleKey = 0;
                    State::settingsWarning = "Warning: Menu Toggle was cleared due to hotkey conflict!";
                }
                else
                {
                    State::settingsWarning = "";
                }
            }

            keybindCode = finalCombo;
            waiting = false;
            State::configDirty = true;
            Log::Debug("Keybind changed for %s to combo: %s (%d)", label, GetHotkeyComboName(keybindCode).c_str(), keybindCode);
        }

        static void DrawDebugTab()
        {
            ImGui::Text("Debug & Environment Info");
            ImGui::Separator();

            ImGui::Text("Graphics API: %s", 
                (State::api == GraphicsAPI::OpenGL3 ? "OpenGL 3" :
                 State::api == GraphicsAPI::D3D9 ? "Direct3D 9" :
                 State::api == GraphicsAPI::D3D11 ? "Direct3D 11" :
                 State::api == GraphicsAPI::D3D12 ? "Direct3D 12" : "Unknown"));
            
            ImGui::Text("Game HWND: 0x%p", State::windowHandle);
            ImGui::Text("Menu state: %s", State::menuOpen ? "Open" : "Closed");

            ImGui::Separator();

            ImGuiIO& io = ImGui::GetIO();
            ImGui::Text("Performance FPS: %.1f", io.Framerate);
            ImGui::Text("Mouse Position: %.1f, %.1f", io.MousePos.x, io.MousePos.y);
            ImGui::Text("Capture Mouse: %s", io.WantCaptureMouse ? "True" : "False");
            ImGui::Text("Capture Keyboard: %s", io.WantCaptureKeyboard ? "True" : "False");
        }

        static void DrawSettingsTab()
        {
            ImGui::Text("Keybinds Configurations");
            ImGui::Separator();

            DrawKeybindControl("Menu Toggle", State::menuToggleKey, State::waitingForMenuToggleKey);
            DrawKeybindControl("Unload Module", State::unloadKey, State::waitingForUnloadKey);

            if (ImGui::Button("Unload now"))
            {
                State::shouldUnload = true;
                Log::Debug("Unload requested from settings button.");
            }

            ImGui::Spacing();
            ImGui::Separator();

            ImGui::Text("Keybind Presets");
            if (ImGui::Button(". / F1"))
            {
                State::menuToggleKey = VK_OEM_PERIOD;
                State::unloadKey = VK_F1;
                State::configDirty = true;
            }

            ImGui::Spacing();
            ImGui::Separator();

            ImGui::Text("Configurations Management");
            if (ImGui::Button("Save Config"))
            {
                ConfigSystem::Save(ConfigSystem::GetConfigPath());
            }
            ImGui::SameLine();
            if (ImGui::Button("Reload Config"))
            {
                ConfigSystem::Load(ConfigSystem::GetConfigPath());
            }

            if (!State::settingsWarning.empty())
            {
                ImGui::Spacing();
                ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), State::settingsWarning.c_str());
            }
        }

        void Draw()
        {
            ImGui::SetNextWindowSize(ImVec2(550.0f, 380.0f), ImGuiCond_FirstUseEver);

            if (!ImGui::Begin("Universal Overlay Menu", &State::menuOpen))
            {
                ImGui::End();
                return;
            }

            if (ImGui::BeginTabBar("UniversalMenuTabBar"))
            {
                // Render client-registered tabs first
                for (const auto& tab : UIRegistry::GetTabs())
                {
                    if (ImGui::BeginTabItem(tab.name.c_str()))
                    {
                        tab.callback();
                        ImGui::EndTabItem();
                    }
                }

                // Render standard SDK tabs
                if (ImGui::BeginTabItem("Settings"))
                {
                    DrawSettingsTab();
                    ImGui::EndTabItem();
                }

                if (ImGui::BeginTabItem("SDK Debug"))
                {
                    DrawDebugTab();
                    ImGui::EndTabItem();
                }

                ImGui::EndTabBar();
            }

            ImGui::End();
        }
    }
}
