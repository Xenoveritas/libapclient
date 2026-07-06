#include "libapclient/logger.h"

#include <format>
#include <string>

#ifdef LIBAPCLIENT_ENABLE_LOGGING

#if defined(WIN32)
#  define NOMINMAX
#  define WIN32_LEAN_AND_MEAN
#  include <windows.h>
#elif defined(__APPLE__)
#  include "macos.h"
#else
#  include <iostream>
#endif

namespace archipelago {

void libapclient_log_message(const std::string& message) {
#if defined(WIN32)
    // On Windows, use OutputDebugString
    OutputDebugStringA(message.c_str());
    // And add a newline
    OutputDebugStringA("\r\n");
#elif defined(__APPLE__)
    macos_log(message.c_str());
#else
    std::cerr << message << std::endl;
#endif
}

}

#endif // LIBAPCLIENT_ENABLE_LOGGING