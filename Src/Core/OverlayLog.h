#pragma once

#include <memory>
#include <string>

namespace spdlog
{
    class logger;
}

namespace UniversalOverlay::Log
{
    enum class LogCategory
    {
        Core,
        Renderer,
        Hooks,
        Ui,
        Assets,
        Fonts,
        Windows,
        Config,
        MarvelRuntime
    };

    bool InitializeLogging(const std::wstring& applicationName);
    void ShutdownLogging();
    std::shared_ptr<spdlog::logger> GetLogger(LogCategory category);
    std::wstring LogPath();

    void Debug(LogCategory category, const char* message);
    void Info(LogCategory category, const char* message);
    void Warn(LogCategory category, const char* message);
    void Error(LogCategory category, const char* message);
}
