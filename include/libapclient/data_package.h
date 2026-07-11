#ifndef _LIBAPCLIENT_DATA_PACKAGE_H

#define _LIBAPCLIENT_DATA_PACKAGE_H

#include "packets.h"
#include "logger.h"

#include <iostream>
#include <string>
#include <unordered_map>

namespace archipelago {

/*! \brief A map of things to their IDs to a thing and also back.
 *
 * This is kind of specialized for GameData's use.
 */
template<typename ID, typename T> class IDMap {
private:
    std::map<ID, T> idToObject;
    std::unordered_map<T, ID> objectToId;
public:
    IDMap() : idToObject(), objectToId() {}
    ~IDMap() {}

    void insert(ID id, T obj) {
        idToObject.insert({ id, obj });
        objectToId.insert({ obj, id });
    }

    /*! \brief Attempt to get an object by and ID.
     * \return a pointer to the object or nullptr if it doesn't exist
     */
    const T* getById(ID id) const {
        auto iter = idToObject.find(id);
        if (iter == idToObject.end()) {
            return nullptr;
        }
        return &(iter->second);
    }

    const std::optional<ID> getByObject(T obj) const  {
        auto iter = objectToId.find(obj);
        if (iter == idToObject.end()) {
            return std::nullopt;
        }
        return iter.get();
    }

    bool empty() const {
        return idToObject.empty();
    }

    std::vector<T> getObjects() {
        std::vector<T> result;
        result.reserve(idToObject.size());
        for (auto& pair : idToObject) {
            result.push_back(pair.second);
        }
        return result;
    }
};

/*! \brief Store data possibly from a packets::DataPackage.
 *
 * The specific game data is defined by the APWorld. This handles
 * `item_name_to_id` and `location_name_to_id`, which map Archipelago names to
 * their internal IDs.
 *
 * However, some games may have additional data provided by the APWorld.
 */
class GameData {
public:
    IDMap<location_id_t, std::string> locations;
    IDMap<item_id_t, std::string> items;
    std::string checksum;

    GameData() : locations(), items(), checksum() {}
    GameData(const json& gamePackage);
    ~GameData() {}

    GameData& operator<<(const json& gamePackage);
    void loadGame(const packets::DataPackage& package, const std::string& gameName);

    std::string getItemName(item_id_t id);
    std::string getLocationName(location_id_t id);

    std::vector<std::string> getItemNames();
    std::vector<std::string> getLocationNames();
};

/*! \brief The default deserializer - always uses BaseDP(const json&) to deserialize.
 */
template<class BaseDP> class BasicDataPackageDeserializer {
public:
    BaseDP deserialize(const std::string& game, const json& dataPackage) {}
};

/*! \brief The key for a data package: the game name and checksum.
 */
struct DataPackageKey {
    std::string gameName;
    std::string checksum;
};

struct DataPackageKeyHash {
    std::size_t operator()(const DataPackageKey& key) const noexcept {
        return std::hash<std::string>()(key.gameName) ^ (std::hash<std::string>()(key.checksum) << 1);
    }
};

/*! \brief A helper class for storing and updating data packages.
 * 
 * Conceptually a data packages should not change between games or runs and is
 * static per APWorld version.
 *
 * The exact contents of a data package are up to the individual APWorld and
 * may contain additional information.
 * 
 * The default template parameters will just drop any additional data and
 * simply take the defaults.
 *
 * \tparam BaseDP the class (or at least base class) used to store a given game's data package
 * \tparam Deserializer a class that handles deserializing data packages
 */
template<class BaseDP = GameData, class Deserializer = BasicDataPackageDeserializer<BaseDP>> class DataPackageCache {
private:
    std::unordered_map<std::string, BaseDP> dataPackages;
    /*! \brief the checksums given by the server for a data package.
     *
     * Maps game name to checksum.
     */
    std::unordered_map<std::string, std::string> checksums;
    Deserializer deserializer;

public:
    /*! \brief Creates a new, empty cache.
     */
    DataPackageCache() : dataPackages(), checksums(), deserializer() {}

    void checkRoomInfo(const packets::RoomInfo& roomInfo) {
        // Check to make sure our checksums match the existing ones
        for (auto dpChecksum : roomInfo.datapackage_checksums) {
            // See if this exists
            auto iter = checksums.find(dpChecksum->first);
            if (iter == checksums.end()) {
                // Not found. Insert a new empty pair
                checksums.insert({ dpChecksum->first, std::string() });
            } else if (dpChecksum->second != *iter) {
                // Checksum does not match. Blank out our game data.
                dataPackages.erase(dpChecksum->first);
            }
        }
    }

    const std::string* getChecksum(const std::string& game) const {
        auto iter = checksums.find(game);
        if (iter == checksums.end()) {
            return nullptr;
        }
        return &(iter->second);
    }

    /*! \brief Gets a list of missing games. These are games where a checksum
     * exists, but no data does.
     * 
     * If the RoomInfo indicated a checksum mismatch, the game will have the
     * game data removed and will be included in this list.
     */
    const std::vector<std::string> findMissingGames() const {
        std::vector<std::string> result;
        // We can be sure this will be no more than this size
        result.reserve(checksums.size());
        for (auto& pair : checksums) {
            // See if there is game data for this checksum
            auto iter = dataPackages.find(pair.first);
            if (iter == dataPackages.end()) {
                // No matching game data - add the game name
                result.push_back(pair.first);
            }
        }
    }

    /*! \brief Update game data with the given JSON data.
     */
    void updateGame(const std::string& gameName, const json& jsonData) {
        // JSON data should have the checksum in it.
        auto iter = jsonData.find("checksum");
        if (iter == jsonData.end()) {
            // oh well
            LIBAPCLIENT_LOG("Missing checksum in data from {}", gameName);
        } else {
            std::string checksum = *iter;
            // Just check to make sure nothing went crazy
            auto iter = checksums.find(gameName);
            if (iter == checksums.end()) {
                // Add it
                checksums.insert({ gameName, checksum });
            } else {
                if (iter->second != checksum) {
                    LIBAPCLIENT_LOG("Checksum mismatch for {} - updating to {} from data package.", gameName, checksum);
                    iter->second = checksum;
                }
            }
        }
        // Insert game data
        dataPackages[gameName] = deserializer.deserialize(jsonData);
    }

    /*! \brief Receive a data package.
     *
     * \param dataPackage the data package packet from the server
     */
    void receiveDataPackage(const packets::DataPackage& dataPackage) {
        for (auto& pair : dataPackage.games) {
            updateGame(pair.first, pair.second);
        }
    }
};

}

#endif // _LIBAPCLIENT_DATA_PACKAGE_H