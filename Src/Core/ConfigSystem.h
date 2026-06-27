#pragma once

#include <functional>
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
            std::wstring lastSerializedValue;
            bool hasLastSerializedValue = false;
        };

        void RegisterBool(const std::string& section, const std::string& key, bool* val, bool defaultVal = false);
        void RegisterFloat(const std::string& section, const std::string& key, float* val, float defaultVal = 0.0f);
        void RegisterInt(const std::string& section, const std::string& key, int* val, int defaultVal = 0);
        void RegisterPostLoadCallback(const std::string& id, std::function<void()> callback);

        void Save(const std::wstring& filePath);
        void Load(const std::wstring& filePath);
        bool SavePreset(int slot);
        bool LoadPreset(int slot);
        std::wstring GetPresetPath(int slot);
        void SaveIfDirty(const std::wstring& filePath);
        void MarkDirty();
        void RefreshDirtyState();
        bool IsConfigLoaded();
        void Clear();

        void SetConfigPath(const std::wstring& path);
        std::wstring GetConfigPath();
    }
}
