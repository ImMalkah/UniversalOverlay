#include "Core/OverlayLog.h"

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>

#include <array>
#include <mutex>
#include <string_view>

#include "spdlog/sinks/rotating_file_sink.h"
#include "spdlog/spdlog.h"

namespace UniversalOverlay::Log
{
    namespace
    {
        constexpr std::size_t kCategoryCount = 9;
        constexpr std::size_t kMaxLogFileSize = 1024 * 1024;
        constexpr std::size_t kMaxLogFiles = 3;

        constexpr std::array<std::string_view, kCategoryCount> kLoggerNames = {
            "core",
            "renderer",
            "hooks",
            "ui",
            "assets",
            "fonts",
            "windows",
            "config",
            "marvel-runtime"
        };

        std::mutex g_logMutex;
        std::array<std::shared_ptr<spdlog::logger>, kCategoryCount> g_loggers;
        std::wstring g_logPath;

        std::size_t CategoryIndex(LogCategory category)
        {
            const auto index = static_cast<std::size_t>(category);
            return index < kCategoryCount ? index : kCategoryCount;
        }

        std::wstring TempRoot()
        {
            const DWORD required = GetTempPathW(0, nullptr);
            if (required == 0)
                return {};

            std::wstring path(required, L'\0');
            const DWORD length = GetTempPathW(required, path.data());
            if (length == 0 || length >= required)
                return {};

            path.resize(length);
            return path;
        }

        std::string Utf8FromWide(const std::wstring& value)
        {
            if (value.empty())
                return {};

            const int required = WideCharToMultiByte(
                CP_UTF8,
                0,
                value.data(),
                static_cast<int>(value.size()),
                nullptr,
                0,
                nullptr,
                nullptr);
            if (required <= 0)
                return {};

            std::string result(static_cast<std::size_t>(required), '\0');
            WideCharToMultiByte(
                CP_UTF8,
                0,
                value.data(),
                static_cast<int>(value.size()),
                result.data(),
                required,
                nullptr,
                nullptr);
            return result;
        }

        std::wstring WideFromUtf8(std::string_view value)
        {
            if (value.empty())
                return {};

            const int required = MultiByteToWideChar(CP_UTF8, 0, value.data(), static_cast<int>(value.size()), nullptr, 0);
            if (required <= 0)
                return {};

            std::wstring result(static_cast<std::size_t>(required), L'\0');
            MultiByteToWideChar(CP_UTF8, 0, value.data(), static_cast<int>(value.size()), result.data(), required);
            return result;
        }

        bool EnsureDirectory(const std::wstring& path)
        {
            if (path.empty())
                return false;

            std::wstring current;
            current.reserve(path.size());

            for (wchar_t ch : path)
            {
                current.push_back(ch);
                if (ch != L'\\' && ch != L'/')
                    continue;

                if (current.size() <= 3)
                    continue;

                if (!CreateDirectoryW(current.c_str(), nullptr) && GetLastError() != ERROR_ALREADY_EXISTS)
                    return false;
            }

            if (!CreateDirectoryW(path.c_str(), nullptr) && GetLastError() != ERROR_ALREADY_EXISTS)
                return false;

            return true;
        }

        std::wstring BuildLogDirectory(const std::wstring& applicationName)
        {
            std::wstring root = TempRoot();
            if (root.empty())
                return {};

            if (!root.empty() && root.back() != L'\\' && root.back() != L'/')
                root.push_back(L'\\');

            root += applicationName;
            root.push_back(L'\\');
            root += std::to_wstring(GetCurrentProcessId());
            root.push_back(L'\\');
            return root;
        }

        void ClearLoggers()
        {
            for (auto& logger : g_loggers)
            {
                if (logger)
                {
                    logger->flush();
                    spdlog::drop(logger->name());
                    logger.reset();
                }
            }
            g_logPath.clear();
        }

        void Write(LogCategory category, spdlog::level::level_enum level, const char* message)
        {
            const auto logger = GetLogger(category);
            if (!logger)
                return;

            logger->log(level, "{}", message != nullptr ? message : "");
        }
    }

    bool InitializeLogging(const std::wstring& applicationName)
    {
        std::lock_guard lock(g_logMutex);
        ClearLoggers();

        const std::wstring logDirectory = BuildLogDirectory(applicationName);
        if (logDirectory.empty() || !EnsureDirectory(logDirectory))
            return false;

        try
        {
            for (std::size_t index = 0; index < kCategoryCount; ++index)
            {
                const std::string name(kLoggerNames[index]);
                const std::wstring logFile = logDirectory + WideFromUtf8(kLoggerNames[index]) + L".log";
                auto sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
                    Utf8FromWide(logFile),
                    kMaxLogFileSize,
                    kMaxLogFiles);
                auto logger = std::make_shared<spdlog::logger>(name, sink);
                logger->set_level(spdlog::level::debug);
                logger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%n] [%l] %v");
                logger->flush_on(spdlog::level::warn);

                spdlog::register_or_replace(logger);
                g_loggers[index] = std::move(logger);
            }
        }
        catch (...)
        {
            ClearLoggers();
            return false;
        }

        g_logPath = logDirectory;
        return true;
    }

    void ShutdownLogging()
    {
        std::lock_guard lock(g_logMutex);
        ClearLoggers();
    }

    std::shared_ptr<spdlog::logger> GetLogger(LogCategory category)
    {
        std::lock_guard lock(g_logMutex);
        const std::size_t index = CategoryIndex(category);
        if (index >= kCategoryCount)
            return nullptr;

        return g_loggers[index];
    }

    std::wstring LogPath()
    {
        std::lock_guard lock(g_logMutex);
        return g_logPath;
    }

    void Debug(LogCategory category, const char* message)
    {
        Write(category, spdlog::level::debug, message);
    }

    void Info(LogCategory category, const char* message)
    {
        Write(category, spdlog::level::info, message);
    }

    void Warn(LogCategory category, const char* message)
    {
        Write(category, spdlog::level::warn, message);
    }

    void Error(LogCategory category, const char* message)
    {
        Write(category, spdlog::level::err, message);
    }
}
