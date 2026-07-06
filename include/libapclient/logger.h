/*! \file logger.h
 * \brief Provides internal logging functions
 */
#ifndef _LIBAPCLIENT_LOGGER_H
#define _LIBAPCLIENT_LOGGER_H

#include <format>
#include <string>

#ifdef LIBAPCLIENT_ENABLE_LOGGING

namespace archipelago {
    void libapclient_log_message(const std::string& message);
    template< class... Args > void libapclient_log(std::format_string<Args...> fmt, Args&&... args) {
        // Basically, dump this on std::vformat
        archipelago::libapclient_log_message(std::vformat(fmt.get(), std::make_format_args(args...)));
    };
}

#define LIBAPCLIENT_LOG archipelago::libapclient_log

#else

// Don't log anything
#define LIBAPCLIENT_LOG(...) do {} while (false)

#endif // LIBAPCLIENT_ENABLE_LOGGING


#endif // _LIBAPCLIENT_LOGGER_H