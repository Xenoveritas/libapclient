/*! \file utils.cpp
 * \brief utility function implementations
 */

#include "libapclient/utils.h"

#include <nlohmann/json.hpp>

#define UUID_SYSTEM_GENERATOR
#include <uuid.h>

#include <iostream>
#include <fstream>
#include <filesystem>
#include <stdexcept>

#ifdef WIN32
#define WIN32_LEAN_AND_MEAN

#include <ShlObj_core.h>
#include <Stringapiset.h>

#elif defined(__APPLE__)

// Include Apple-specific stubs
#include "macos.h"

#else

#include <cstdlib>

#endif

using json = nlohmann::json;

namespace archipelago {

std::filesystem::path getPlayerArchipelagoCacheDirectory() {
    // This differs per OS. Sigh.
#if defined(WIN32)
    PWSTR localAppDataPath = NULL;
    if (SHGetKnownFolderPath(FOLDERID_LocalAppData, KF_FLAG_DEFAULT, NULL, &localAppDataPath) != S_OK) {
        // ... uh oh
        std::cerr << "Unable to find local path (SHGetKnownFolderPath failed)" << std::endl;
        // API says it still needs to be freed. If it was never set, sending NULL should have no effect, so this should be safe
        CoTaskMemFree(localAppDataPath);
        throw std::runtime_error("SHGetKnownFolderPath lookup failed");
    }
    // Path will handle character encoding for us
    std::filesystem::path result{ std::wstring(localAppDataPath) };
    // Free the Windows buffer
    CoTaskMemFree(localAppDataPath);
    // Now just append \Archipelago\Cache:
    result /= std::string("Archipelago\\Cache");
#elif defined(__APPLE__)
    // Grab the path from macOS
    std::u16string cacheDir;
    macos_get_cache_path(cacheDir);
    std::filesystem::path result(cacheDir);
    result /= "Archipelago";
#else
    // Get it via the environment
    std::filesystem::path result{ std::getenv("HOME") };
    // getenv will get an empty string if no HOME, and /.cache/Archipelago is
    // (probably) fine for that case
    result /= ".cache/Archipelago";
#endif
    return result;
}

/*! \brief Get the common.json path where the UUID is stored for the player.
 *
 * This is just:
 * ```cpp
 * getPlayerArchipelagoCacheDirectory() / "common.json";
 * ```
 */
std::filesystem::path getArchipelagoCacheCommonPath() {
    return getPlayerArchipelagoCacheDirectory() / "common.json";
}

std::optional<std::string> getCachedPlayerUUID() noexcept {
    try {
        std::ifstream commonIn(getArchipelagoCacheCommonPath(), std::ios::binary);
        if (!commonIn) {
            // In this case, we weren't able to read the file.
            // It may simply not exist. In any case, the player UUID isn't available.
            return std::nullopt;
        }
        json cacheJson;
        commonIn >> cacheJson;
        return (std::string) cacheJson.at("uuid");
    } catch (const json::exception&) {
        return std::nullopt;
#if defined(WIN32)
    } catch (const std::runtime_error&) {
        // If the cache directory can't be found, there's no way to find the
        // cached UUID.
        // Currently this exception is only possible on Windows.
        return std::nullopt;
#endif
    }
}

std::string getPlayerUUID(bool updateCache) noexcept {
    // First, do this the hopefully easy way
    auto cachedUUID = getCachedPlayerUUID();
    if (cachedUUID.has_value()) {
        return *cachedUUID;
    }
    // Well rats. Generate a new one.
    uuids::uuid const id = uuids::uuid_system_generator{}();
    std::string uuid_str = uuids::to_string(id);
    if (updateCache) {
        try {
            auto commonJsonPath = getArchipelagoCacheCommonPath();
            auto fs = std::filesystem::status(commonJsonPath);
            bool copyExisting = std::filesystem::exists(fs) && std::filesystem::is_regular_file(fs);
            if (!copyExisting) {
                // May also need to create the path.
                auto parentPath = commonJsonPath.parent_path();
                if (!std::filesystem::exists(parentPath)) {
                    std::filesystem::create_directories(parentPath);
                }
            }
            std::fstream commonStream(commonJsonPath, commonStream.binary | (copyExisting ? commonStream.in : 0) | commonStream.out);
            commonStream.exceptions(commonStream.badbit | commonStream.failbit);
            json cacheJson;
            if (copyExisting) {
                commonStream >> cacheJson;
                if (!cacheJson.is_object()) {
                    // If the common.json file exists and the cache JSON is
                    // invalid, just give up immediately.
                    return uuid_str;
                }
                // if here, we have an object we can use, so truncate the file
                std::filesystem::resize_file(commonJsonPath, 0);
                commonStream.seekp(0);
            } else {
                cacheJson = json::object();
            }
            cacheJson["uuid"] = uuid_str;
            commonStream << cacheJson;
        } catch (const json::exception& e) {
            std::cerr << "Not updating Archipelago cache: could not parse existing Archipelago common.json: " << e.what() << std::endl;
        } catch (const std::runtime_error& e) {
            // Log but otherwise ignore
            std::cerr << "Unable to update Archipelago UUID: " << e.what() << std::endl;
        }
    }
    return uuid_str;
}

} // namespace archipelago