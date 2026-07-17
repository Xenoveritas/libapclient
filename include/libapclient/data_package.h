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
        if (iter == objectToId.end()) {
            return std::nullopt;
        }
        return iter->second;
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
struct GameData {
    IDMap<location_id_t, std::string> locations{};
    IDMap<item_id_t, std::string> items{};
    std::string checksum{};

    GameData& operator<<(const json& gamePackage);
    void loadGame(const packets::DataPackage& package, const std::string& gameName);

    /*! \brief Gets the item name for the given ID.
     *
     * If no name exists for this item ID, this will return "Item<id>".
     * (The assumption is that any ID used is expected to exist, and if it
     * doesn't, an error has occurred generating the game data. Use
     * hasItemId(item_id_t) to check if the item exists in this data package.)
     *
     * \param id the item ID to lookup
     * \return the item name for that ID and a default if it doesn't exist
     */
    std::string getItemName(item_id_t id);

    /*! \brief Gets the location name for the given ID.
     *
     * If no name exists for this location ID, this will return "Location<id>".
     * (The assumption is that any ID used is expected to exist, and if it
     * doesn't, an error has occurred generating the game data. Use
     * hasLocationId(item_id_t) to check if the location exists in this data
     * package.)
     *
     * \param id the location ID to lookup
     * \return the location name for that ID and a default if it doesn't exist
     */
    std::string getLocationName(location_id_t id);

    /*! \brief Check if an item name exists for the given ID.
     *
     * \param id the item ID to lookup
     * \return true if a name exists for the given item ID
     */
    bool hasItemId(item_id_t id);

    /*! \brief Check if a location name exists for the given ID.
     *
     * \param id the location ID to lookup
     * \return true if a name exists for the given location ID
     */
    bool hasLocationId(item_id_t id);

    /*! \brief Gets the item ID for the given item name.
     *
     * If the item doesn't exist, the optional is `std::nullopt`.
     *
     * \param itemName the item name to lookup
     * \return the item ID if it exists
     */
    std::optional<item_id_t> getItemID(const std::string& itemName);

    /*! \brief Gets the location ID for the given location name.
     *
     * If the item doesn't exist, the optional is `std::nullopt`.
     *
     * \param locationName the location name to lookup
     * \return the location ID if it exists
     */
    std::optional<location_id_t> getLocationID(const std::string& locationName);

    std::vector<std::string> getItemNames();
    std::vector<std::string> getLocationNames();
};

void from_json(const json& j, GameData& data);

/*! \brief The default deserializer.
 *
 * This uses `static_cast<BaseDP>(dataPackage)` to deserialize the data package
 * from the JSON, which effectively uses `from_json(const json&, BaseDP)`.
 */
template<class BaseDP> class BasicDataPackageDeserializer {
public:
    BaseDP deserialize(const std::string& game, const json& dataPackage) {
        return static_cast<BaseDP>(dataPackage);
    }
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

    /*! \brief Gets the number of data packages cached. This is not necessarily
     * the same as the number of checksums known.
     */
    size_t size() {
        return dataPackages.size();
    }

    size_t knownChecksumCount() {
        return checksums.size();
    }

    void checkRoomInfo(const packets::RoomInfo& roomInfo) {
        // Check to make sure our checksums match the existing ones
        for (auto& dpPair : roomInfo.datapackage_checksums) {
            // See if this exists
            auto iter = checksums.find(dpPair.first);
            if (iter == checksums.end()) {
                // Not found. Insert a new empty pair
                checksums.insert({ dpPair.first, dpPair.second });
            } else if (dpPair.second != iter->first) {
                // Checksum does not match. Blank out our game data.
                dataPackages.erase(dpPair.first);
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
        return result;
    }

    /*! \brief Gets the list of all known games.
     *
     * This is the list of games which have checksums stored for them.
     */
    const std::vector<std::string> getGames() const {
        std::vector<std::string> result;
        result.reserve(checksums.size());
        for (auto& pair : checksums) {
            result.push_back(pair.first);
        }
        return result;
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

/*! \brief Basic DataPackageCache that stores game data in a GameData.
 */
typedef DataPackageCache<GameData> GameDataCache;

}

#endif // _LIBAPCLIENT_DATA_PACKAGE_H