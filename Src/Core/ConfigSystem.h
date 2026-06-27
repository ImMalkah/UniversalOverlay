#pragma once

#include <string>
#include <vector>

namespace UniversalOverlay
{
    namespace ConfigSystem
    {
        enum class ConfigType
        {
            Bool,
            Float,
            Int
        };

        struct ConfigEntry
        {
            std::string section;
            std::string key;
            ConfigType type;
            void* ptr;
            union {
                bool defaultBool;
                float defaultFloat;
                int defaultInt;
            };
        };

        void RegisterBool(const std::string& section, const std::string& key, bool* val, bool defaultVal = false);
        void RegisterFloat(const std::string& section, const std::string& key, float* val, float defaultVal = 0.0f);
        void RegisterInt(const std::string& section, const std::string& key, int* val, int defaultVal = 0);

        void Save(const std::wstring& filePath);
        void Load(const std::wstring& filePath);
        void SaveIfDirty(const std::wstring& filePath);
        void Clear();

        void SetConfigPath(const std::wstring& path);
        std::wstring GetConfigPath();
    }
}
