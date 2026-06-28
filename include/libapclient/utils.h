/*! \file utils.h
 * \brief Utility functions related to the Archipelago client.
 */
// These are separate from libapclient.h more for organization reasons than anything else.

#ifndef LIBAPCLIENT_UTILS_H_
#define LIBAPCLIENT_UTILS_H_

#include <string>
#include <optional>
#include <filesystem>

namespace archipelago {

/*! \brief Gets the Archipelago cache directory.
 *
 * See the Python package
 * <a href="https://github.com/tox-dev/platformdirs">platformdirs</a> for
 * details; this is effectively:
 *
 * ```python
 * platformdirs.user_cache_dir("Archipelago", False)
 * ```
 *
 * This can throw std::runtime_error if the lookup fails, which should mostly
 * never happen but can if the runtime environment is insane. (Specifically,
 * things like Windows not having LocalAppData as a known folder.)
 *
 * This does not guarantee that the directory exists, it merely generates the
 * expected path to it.
 *
 * \return the path to the user's Archipelago cache directory
 */
std::filesystem::path getPlayerArchipelagoCacheDirectory();

/*! \brief Look up the UUID for a player within the Archipelago cache data.
 *
 * This involves a file lookup and therefore can block on file I/O. Exceptions
 * are caught.
 *
 * \return the player's cached UUID if it exists, or `std::nullopt` if it could
 *         not be looked up (no cache, cached JSON is corrupt, whatever)
 */
std::optional<std::string> getCachedPlayerUUID() noexcept;

/*! \brief Gets a UUID for the player, creating one if necessary.
 *
 * First attempts to get the cached player UUID via getCachedPlayerUUID(). If
 * that returns `std::nullopt`, this will instead generate a new UUID and return
 * that. The method for generating a new UUID is platform-specific.
 *
 * \param updateCache whether to write the UUID to the Archipelago cache if
 *        possible
 * \return a UUID for the player
 */
std::string getPlayerUUID(bool updateCache) noexcept;

}

#endif // LIBAPCLIENT_UTILS_H_