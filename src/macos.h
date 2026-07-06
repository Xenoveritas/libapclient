// macOS-specific functions.
// Here because they're not public API
#ifndef _LIBAPCLIENT_OSX_H
#define _LIBAPCLIENT_OSX_H

#include <string>

void macos_log(const std::string& message);

void macos_get_cache_path(std::u16string& str);

#endif // _LIBAPCLIENT_OSX_H