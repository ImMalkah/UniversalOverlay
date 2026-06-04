#pragma once

#include <Windows.h>
#include <cstdio>
#include <cstdarg>

namespace UniversalOverlay
{
    namespace Log
    {
        inline void Debug(const char* fmt, ...)
        {
            char buffer[1024];

            va_list args;
            va_start(args, fmt);
            vsnprintf(buffer, sizeof(buffer), fmt, args);
            va_end(args);

            // Output to debugger
            OutputDebugStringA("[UniversalOverlay] ");
            OutputDebugStringA(buffer);
            OutputDebugStringA("\n");

            // Also output to console stdout
            printf("[UniversalOverlay] %s\n", buffer);
        }
    }
}
