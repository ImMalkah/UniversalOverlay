#include "Core/ConfigSystem.h"
#include "Core/CoreState.h"
#include "Core/Log.h"
#include <Windows.h>

namespace UniversalOverlay
{
    namespace ConfigSystem
    {
        static std::vector<ConfigEntry> g_entries;
        static std::wstring g_configPath = L"overlay_config.ini";

        static std::wstring ToWString(const std::string& str)
        {
            if (str.empty()) return L"";
            int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
            std::wstring wstrTo(size_needed, 0);
            MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
            return wstrTo;
        }

        void RegisterBool(const std::string& section, const std::string& key, bool* val, bool defaultVal)
        {
            ConfigEntry entry;
            entry.section = section;
            entry.key = key;
            entry.type = ConfigType::Bool;
            entry.ptr = val;
            entry.defaultBool = defaultVal;
            g_entries.push_back(entry);
            *val = defaultVal;
        }

        void RegisterFloat(const std::string& section, const std::string& key, float* val, float defaultVal)
        {
            ConfigEntry entry;
            entry.section = section;
            entry.key = key;
            entry.type = ConfigType::Float;
            entry.ptr = val;
            entry.defaultFloat = defaultVal;
            g_entries.push_back(entry);
            *val = defaultVal;
        }

        void RegisterInt(const std::string& section, const std::string& key, int* val, int defaultVal)
        {
            ConfigEntry entry;
            entry.section = section;
            entry.key = key;
            entry.type = ConfigType::Int;
            entry.ptr = val;
            entry.defaultInt = defaultVal;
            g_entries.push_back(entry);
            *val = defaultVal;
        }

        void Save(const std::wstring& filePath)
        {
            g_configPath = filePath;
            for (const auto& entry : g_entries)
            {
                std::wstring wSection = ToWString(entry.section);
                std::wstring wKey = ToWString(entry.key);

                if (entry.type == ConfigType::Bool)
                {
                    bool val = *reinterpret_cast<bool*>(entry.ptr);
                    WritePrivateProfileStringW(wSection.c_str(), wKey.c_str(), val ? L"1" : L"0", filePath.c_str());
                }
                else if (entry.type == ConfigType::Float)
                {
                    float val = *reinterpret_cast<float*>(entry.ptr);
                    wchar_t buf[64];
                    swprintf_s(buf, L"%.4f", val);
                    WritePrivateProfileStringW(wSection.c_str(), wKey.c_str(), buf, filePath.c_str());
                }
                else if (entry.type == ConfigType::Int)
                {
                    int val = *reinterpret_cast<int*>(entry.ptr);
                    wchar_t buf[64];
                    swprintf_s(buf, L"%d", val);
                    WritePrivateProfileStringW(wSection.c_str(), wKey.c_str(), buf, filePath.c_str());
                }
            }

            State::configDirty = false;
            Log::Debug("Configurations successfully saved.");
        }

        void Load(const std::wstring& filePath)
        {
            g_configPath = filePath;
            for (const auto& entry : g_entries)
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

            State::configLoaded = true;
            State::configDirty = false;
            Log::Debug("Configurations successfully loaded.");
        }

        void SaveIfDirty(const std::wstring& filePath)
        {
            if (State::configDirty)
            {
                Save(filePath);
            }
        }

        void Clear()
        {
            g_entries.clear();
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
