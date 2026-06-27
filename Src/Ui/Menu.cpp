#include "Ui/Menu.h"
#include "Core/CoreState.h"
#include "Ui/OverlayGallery.h"
#include "Ui/UIRegistry.h"
#include "Core/ConfigSystem.h"
#include "Core/Log.h"
#include "imgui.h"

#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>

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

        static bool HasMeaningfulDelta(float lhs, float rhs)
        {
            return std::fabs(lhs - rhs) > 0.5f;
        }

        static void CaptureMenuPlacement()
        {
            const ImVec2 position = ImGui::GetWindowPos();
            const ImVec2 size = ImGui::GetWindowSize();

            if (HasMeaningfulDelta(State::menuPositionX, position.x) ||
                HasMeaningfulDelta(State::menuPositionY, position.y) ||
                HasMeaningfulDelta(State::menuSizeX, size.x) ||
                HasMeaningfulDelta(State::menuSizeY, size.y))
            {
                State::menuPositionX = position.x;
                State::menuPositionY = position.y;
                State::menuSizeX = size.x;
                State::menuSizeY = size.y;
                ConfigSystem::MarkDirty();
            }
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
                ConfigSystem::MarkDirty();
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
            ConfigSystem::MarkDirty();
            Log::Debug("Keybind changed for %s to combo: %s (%d)", label, GetHotkeyComboName(keybindCode).c_str(), keybindCode);
        }

        static bool DrawDangerButton(const char* label)
        {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.58f, 0.08f, 0.10f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.78f, 0.12f, 0.14f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.95f, 0.16f, 0.18f, 1.0f));
            const bool clicked = ImGui::Button(label);
            ImGui::PopStyleColor(3);
            return clicked;
        }

        static void DrawDebugTab()
        {
            ImGui::Text("Debug & Environment Info");
            ImGui::Separator();

            ImGui::Text("Graphics API: %s", 
                (State::api == GraphicsAPI::OpenGL3 ? "OpenGL 3" :
                 State::api == GraphicsAPI::D3D9 ? "Direct3D 9" :
                 State::api == GraphicsAPI::D3D10 ? "Direct3D 10" :
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

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Text("Danger Zone");
            if (DrawDangerButton("Emergency Unload"))
            {
                State::shouldUnload = true;
                Log::Debug("Emergency unload requested from SDK debug button.");
            }
        }

        static void DrawPresetControls()
        {
            static constexpr const char* kPresetLabels[] = {
                "Preset 1",
                "Preset 2",
                "Preset 3",
                "Preset 4",
                "Preset 5"
            };

            ImGui::Text("Configuration Presets");
            for (int slot = 1; slot <= 5; ++slot)
            {
                ImGui::PushID(slot);
                ImGui::TextUnformatted(kPresetLabels[slot - 1]);
                ImGui::SameLine(120.0f);
                if (ImGui::Button("Save"))
                {
                    if (ConfigSystem::SavePreset(slot))
                        State::settingsWarning = std::string(kPresetLabels[slot - 1]) + " saved.";
                    else
                        State::settingsWarning = std::string(kPresetLabels[slot - 1]) + " could not be saved.";
                }

                ImGui::SameLine();
                if (ImGui::Button("Load"))
                {
                    if (ConfigSystem::LoadPreset(slot))
                        State::settingsWarning = std::string(kPresetLabels[slot - 1]) + " loaded.";
                    else
                        State::settingsWarning = std::string(kPresetLabels[slot - 1]) + " does not exist yet.";
                }

                ImGui::PopID();
            }
        }

        static void DrawSettingsTab()
        {
            ImGui::Text("Keybinds Configurations");
            ImGui::Separator();

            DrawKeybindControl("Menu Toggle", State::menuToggleKey, State::waitingForMenuToggleKey);
            DrawKeybindControl("Unload Module", State::unloadKey, State::waitingForUnloadKey);

            ImGui::Spacing();
            ImGui::Separator();

            for (const SettingsSection& section : UIRegistry::GetSettingsSections())
            {
                if (!section.callback)
                    continue;

                ImGui::Text("%s", section.name.c_str());
                ImGui::Separator();
                section.callback();
                ImGui::Spacing();
                ImGui::Separator();
            }

            DrawPresetControls();

            ImGui::Spacing();
            ImGui::Separator();

            ImGui::Text("Configurations Management");
            if (ImGui::Button("Save Config"))
            {
                ConfigSystem::RefreshDirtyState();
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
                ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "%s", State::settingsWarning.c_str());
            }
        }

        static void DrawEBookTab()
        {
            ImGui::Text("UniversalOverlay e-book");
            ImGui::Separator();

            ImGui::TextWrapped("Settings is a built-in SDK tab. It always includes menu hotkeys, full configuration save/reload, and five configuration preset slots.");
            ImGui::Spacing();
            ImGui::TextWrapped("Host projects can add their own Settings sections with UniversalOverlay::RegisterSettingsSection(). Register those values with RegisterConfigBool, RegisterConfigFloat, or RegisterConfigInt so normal saves and presets include them.");
            ImGui::Spacing();
            ImGui::TextWrapped("Presets save every registered config entry to a sibling preset INI file, then load those same entries back through the config system.");
            ImGui::Spacing();
            ImGui::TextWrapped("SDK Debug owns emergency actions and environment diagnostics. Use Emergency Unload only when the overlay needs to shut down immediately.");
            ImGui::Spacing();
            ImGui::TextWrapped("Tabs stay fixed at the top of the menu. Large panels scroll inside the selected tab area so navigation remains visible.");
        }

        void Draw()
        {
            const ImGuiCond placementCondition = State::applySavedMenuPlacement ? ImGuiCond_Always : ImGuiCond_FirstUseEver;
            ImGui::SetNextWindowPos(ImVec2(State::menuPositionX, State::menuPositionY), placementCondition);
            ImGui::SetNextWindowSize(ImVec2(State::menuSizeX, State::menuSizeY), placementCondition);

            if (!ImGui::Begin("Universal Overlay Menu", &State::menuOpen))
            {
                CaptureMenuPlacement();
                State::applySavedMenuPlacement = false;
                ImGui::End();
                return;
            }

            CaptureMenuPlacement();
            State::applySavedMenuPlacement = false;

            TabCallback selectedTab;

            if (ImGui::BeginTabBar("UniversalMenuTabBar"))
            {
                // Render client-registered tabs first
                for (const auto& tab : UIRegistry::GetTabs())
                {
                    if (ImGui::BeginTabItem(tab.name.c_str()))
                    {
                        selectedTab = tab.callback;
                        ImGui::EndTabItem();
                    }
                }

                // Render standard SDK tabs
                if (ImGui::BeginTabItem("UI Gallery"))
                {
                    selectedTab = Ui::DrawGallery;
                    ImGui::EndTabItem();
                }

                if (ImGui::BeginTabItem("Settings"))
                {
                    selectedTab = DrawSettingsTab;
                    ImGui::EndTabItem();
                }

                if (ImGui::BeginTabItem("e-book"))
                {
                    selectedTab = DrawEBookTab;
                    ImGui::EndTabItem();
                }

                if (ImGui::BeginTabItem("SDK Debug"))
                {
                    selectedTab = DrawDebugTab;
                    ImGui::EndTabItem();
                }

                ImGui::EndTabBar();
            }

            ImGui::Separator();
            if (ImGui::BeginChild("UniversalMenuScrollableContent", ImVec2(0.0f, 0.0f), false, ImGuiWindowFlags_AlwaysVerticalScrollbar))
            {
                if (selectedTab)
                    selectedTab();
            }
            ImGui::EndChild();

            ImGui::End();
        }
    }
}
