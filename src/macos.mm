#import <Foundation/Foundation.h>
#import <OSLog/OSLog.h>

#include <stdexcept>

#include "macos.h"

void macos_log(const std::string& message) {
    os_log(OS_LOG_DEFAULT, "%{public}s", message.c_str());
}

void macos_get_cache_path(std::u16string& str) {
    NSArray* result = NSSearchPathForDirectoriesInDomains(NSCachesDirectory, NSUserDomainMask, YES);
    if ([result count] < 1) {
        // oops
        throw std::runtime_error("Unable to locate cache path");
    }
    // Copy the result into the given string
    NSString* path = [result firstObject];
    // Size the string to match
    str.resize([path length]);
    // And copy
    [path getCharacters: reinterpret_cast<unichar*>(str.data()) range: NSMakeRange(0, [path length])];
    [result release];
}