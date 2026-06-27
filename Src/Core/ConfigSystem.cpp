#include "Core/ConfigSystem.h"
#include "Core/CoreState.h"
#include "Core/Log.h"
#include <Windows.h>
#include <utility>

namespace UniversalOverlay
{
    namespace ConfigSystem
    {
        struct PostLoadCallback
        {
            std::string id;
            std::function<void()> callback;
        };

        static std::vector<ConfigEntry> g_entries;
        static std::vector<PostLoadCallback> g_postLoadCallbacks;
        static std::wstring g_configPath = L"overlay_config.ini";

        static bool IsValidPresetSlot(int slot)
        {
            return slot >= 1 && slot <= 5;
        }

        static std::wstring BuildPresetPath(int slot)
        {
            if (!IsValidPresetSlot(slot))
                return L"";

            const std::wstring basePath = g_configPath.empty() ? L"overlay_config.ini" : g_configPath;
            const std::size_t slash = basePath.find_last_of(L"\\/");
            const std::size_t dot = basePath.find_last_of(L'.');
            const bool hasExtension = dot != std::wstring::npos && (slash == std::wstring::npos || dot > slash);

            static constexpr const wchar_t* kPresetSuffixes[] = {
                L".preset1",
                L".preset2",
                L".preset3",
                L".preset4",
                L".preset5"
            };
            const wchar_t* suffix = kPresetSuffixes[slot - 1];

            if (!hasExtension)
                return basePath + suffix + L".ini";

            return basePath.substr(0, dot) + suffix + basePath.substr(dot);
        }

        static std::wstring ToWString(const std::string& str)
        {
            if (str.empty()) return L"";
            int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
            std::wstring wstrTo(size_needed, 0);
            MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
            return wstrTo;
        }

        static ConfigEntry* FindEntry(const std::string& section, const std::string& key, ConfigType type)
        {
            for (ConfigEntry& entry : g_entries)
            {
                if (entry.section == section && entry.key == key && entry.type == type)
                    return &entry;
            }

            return nullptr;
        }

        static std::wstring SerializeEntryValue(const ConfigEntry& entry)
        {
            wchar_t buf[64] = {};

            if (entry.type == ConfigType::Bool)
            {
                const bool val = *reinterpret_cast<bool*>(entry.ptr);
                return val ? L"1" : L"0";
            }

            if (entry.type == ConfigType::Float)
            {
                const float val = *reinterpret_cast<float*>(entry.ptr);
                swprintf_s(buf, L"%.4f", val);
                return buf;
            }

            const int val = *reinterpret_cast<int*>(entry.ptr);
            swprintf_s(buf, L"%d", val);
            return buf;
        }

        static void UpdateLastSerializedValue(ConfigEntry& entry)
        {
            entry.lastSerializedValue = SerializeEntryValue(entry);
            entry.hasLastSerializedValue = true;
        }

        static void ApplyDefaultValue(ConfigEntry& entry)
        {
            if (entry.type == ConfigType::Bool)
                *reinterpret_cast<bool*>(entry.ptr) = entry.defaultBool;
            else if (entry.type == ConfigType::Float)
                *reinterpret_cast<float*>(entry.ptr) = entry.defaultFloat;
            else if (entry.type == ConfigType::Int)
                *reinterpret_cast<int*>(entry.ptr) = entry.defaultInt;
        }

        static void LoadRegisteredEntry(ConfigEntry& entry, const std::wstring& filePath)
        {
            std::wstring wSection = ToWString(entry.section);
            std::wstring wKey = ToWString(entry.key);

            if (entry.type == ConfigType::Bool)
            {
                int val = GetPrivateProfileIntW(
                    wSection.c_str(),
                    wKey.c_str(),
                    entry.defaultBool ? 1 : 0,
                    filePath.c_str()
                );
                *reinterpret_cast<bool*>(entry.ptr) = (val != 0);
            }
            else if (entry.type == ConfigType::Float)
            {
                wchar_t defaultBuf[64];
                swprintf_s(defaultBuf, L"%.4f", entry.defaultFloat);

                wchar_t buf[64] = {};
                GetPrivateProfileStringW(
                    wSection.c_str(),
                    wKey.c_str(),
                    defaultBuf,
                    buf,
                    sizeof(buf) / sizeof(wchar_t),
                    filePath.c_str()
                );
                *reinterpret_cast<float*>(entry.ptr) = static_cast<float>(_wtof(buf));
            }
            else if (entry.type == ConfigType::Int)
            {
                int val = GetPrivateProfileIntW(
                    wSection.c_str(),
                    wKey.c_str(),
                    entry.defaultInt,
                    filePath.c_str()
                );
                *reinterpret_cast<int*>(entry.ptr) = val;
            }
        }

        static void WriteRegisteredEntry(const ConfigEntry& entry, const std::wstring& filePath)
        {
            std::wstring wSection = ToWString(entry.section);
            std::wstring wKey = ToWString(entry.key);
            const std::wstring value = SerializeEntryValue(entry);
            WritePrivateProfileStringW(wSection.c_str(), wKey.c_str(), value.c_str(), filePath.c_str());
        }

        static void RegisterEntry(ConfigEntry entry)
        {
            if (ConfigEntry* existing = FindEntry(entry.section, entry.key, entry.type))
            {
                existing->ptr = entry.ptr;
                if (entry.type == ConfigType::Bool)
                    existing->defaultBool = entry.defaultBool;
                else if (entry.type == ConfigType::Float)
                    existing->defaultFloat = entry.defaultFloat;
                else if (entry.type == ConfigType::Int)
                    existing->defaultInt = entry.defaultInt;

                if (State::configLoaded)
                    LoadRegisteredEntry(*existing, g_configPath);
                else
                    ApplyDefaultValue(*existing);

                UpdateLastSerializedValue(*existing);
                return;
            }

            g_entries.push_back(entry);
            ConfigEntry& registered = g_entries.back();

            if (State::configLoaded)
                LoadRegisteredEntry(registered, g_configPath);
            else
                ApplyDefaultValue(registered);

            UpdateLastSerializedValue(registered);
        }

        void RegisterBool(const std::string& section, const std::string& key, bool* val, bool defaultVal)
        {
            ConfigEntry entry;
            entry.section = section;
            entry.key = key;
            entry.type = ConfigType::Bool;
            entry.ptr = val;
            entry.defaultBool = defaultVal;
            RegisterEntry(entry);
        }

        void RegisterFloat(const std::string& section, const std::string& key, float* val, float defaultVal)
        {
            ConfigEntry entry;
            entry.section = section;
            entry.key = key;
            entry.type = ConfigType::Float;
            entry.ptr = val;
            entry.defaultFloat = defaultVal;
            RegisterEntry(entry);
        }

        void RegisterInt(const std::string& section, const std::string& key, int* val, int defaultVal)
        {
            ConfigEntry entry;
            entry.section = section;
            entry.key = key;
            entry.type = ConfigType::Int;
            entry.ptr = val;
            entry.defaultInt = defaultVal;
            RegisterEntry(entry);
        }

        void RegisterPostLoadCallback(const std::string& id, std::function<void()> callback)
        {
            if (id.empty() || !callback)
                return;

            for (PostLoadCallback& registered : g_postLoadCallbacks)
            {
                if (registered.id == id)
                {
                    registered.callback = std::move(callback);
                    if (State::configLoaded)
                        registered.callback();
                    return;
                }
            }

            g_postLoadCallbacks.push_back({ id, std::move(callback) });
            if (State::configLoaded)
                g_postLoadCallbacks.back().callback();
        }

        void Save(const std::wstring& filePath)
        {
            g_configPath = filePath;
            for (ConfigEntry& entry : g_entries)
            {
                WriteRegisteredEntry(entry, filePath);
                UpdateLastSerializedValue(entry);
            }

            State::configDirty = false;
            Log::Debug("Configurations successfully saved.");
        }

        void Load(const std::wstring& filePath)
        {
            g_configPath = filePath;
            for (ConfigEntry& entry : g_entries)
            {
                LoadRegisteredEntry(entry, filePath);
                UpdateLastSerializedValue(entry);
            }

            State::configLoaded = true;
            State::configDirty = false;
            for (const PostLoadCallback& registered : g_postLoadCallbacks)
            {
                if (registered.callback)
                    registered.callback();
            }

            Log::Debug("Configurations successfully loaded.");
        }

        bool SavePreset(int slot)
        {
            const std::wstring presetPath = BuildPresetPath(slot);
            if (presetPath.empty())
            {
                Log::Debug("Config preset save rejected for invalid slot: %d", slot);
                return false;
            }

            for (const ConfigEntry& entry : g_entries)
            {
                WriteRegisteredEntry(entry, presetPath);
            }

            Log::Debug("Config preset saved for slot: %d", slot);
            return true;
        }

        bool LoadPreset(int slot)
        {
            const std::wstring presetPath = BuildPresetPath(slot);
            if (presetPath.empty())
            {
                Log::Debug("Config preset load rejected for invalid slot: %d", slot);
                return false;
            }

            const DWORD attributes = GetFileAttributesW(presetPath.c_str());
            if (attributes == INVALID_FILE_ATTRIBUTES || (attributes & FILE_ATTRIBUTE_DIRECTORY) != 0)
            {
                Log::Debug("Config preset load skipped because preset file is missing for slot: %d", slot);
                return false;
            }

            for (ConfigEntry& entry : g_entries)
            {
                LoadRegisteredEntry(entry, presetPath);
            }

            State::configLoaded = true;
            State::configDirty = true;
            for (const PostLoadCallback& registered : g_postLoadCallbacks)
            {
                if (registered.callback)
                    registered.callback();
            }

            Log::Debug("Config preset loaded for slot: %d", slot);
            return true;
        }

        std::wstring GetPresetPath(int slot)
        {
            return BuildPresetPath(slot);
        }

        void SaveIfDirty(const std::wstring& filePath)
        {
            RefreshDirtyState();
            if (State::configDirty)
            {
                Save(filePath);
            }
        }

        void MarkDirty()
        {
            State::configDirty = true;
        }

        void RefreshDirtyState()
        {
            if (State::configDirty)
                return;

            for (const ConfigEntry& entry : g_entries)
            {
                if (!entry.hasLastSerializedValue || entry.lastSerializedValue != SerializeEntryValue(entry))
                {
                    State::configDirty = true;
                    return;
                }
            }
        }

        bool IsConfigLoaded()
        {
            return State::configLoaded;
        }

        void Clear()
        {
            g_entries.clear();
            g_postLoadCallbacks.clear();
            State::configLoaded = false;
            State::configDirty = false;
        }

        void SetConfigPath(const std::wstring& path)
        {
            g_configPath = path;
        }

        std::wstring GetConfigPath()
        {
            return g_configPath;
        }
    }
}
