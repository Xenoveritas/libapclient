#include "libapclient/tracker.h"
#include "libapclient/data_package.h"
#include "libapclient/logger.h"

namespace archipelago {

template<typename ID> void populate(IDMap<ID, std::string>& map, const json& j) {
    if (j.is_object()) {
        // iterate through key/values
        for (auto& item : j.items()) {
            if (item.value().is_number_integer()) {
                // Add it
                map.insert(static_cast<ID>(item.value()), item.key());
            }
        }
    }
}

GameData::GameData(const json& gamePackage) : locations(), items(), checksum() {
    operator<<(gamePackage);
}

GameData& GameData::operator<<(const json& gamePackage) {
    // If the game package isn't a JSON object, just don't do anything special.
    if (!gamePackage.is_object()) {
        LIBAPCLIENT_LOG("Ignoring {}: not an object", gamePackage.dump());
        return *this;
    }
    // Should have two things in it: "item_name_to_id" and "location_name_to_id"
    auto iter = gamePackage.find("item_name_to_id");
    if (iter == gamePackage.end()) {
        LIBAPCLIENT_LOG("Could not find item_name_to_id");
    } else {
        populate<item_id_t>(items, *iter);
    }
    iter = gamePackage.find("location_name_to_id");
    if (iter != gamePackage.end()) {
        populate<item_id_t>(locations, *iter);
    }
    return *this;
}

void GameData::loadGame(const packets::DataPackage& package, const std::string& gameName) {
    auto iter = package.games.find(gameName);
    if (iter != package.games.end()) {
        operator<<(iter->second);
    }
}

std::string GameData::getItemName(item_id_t id) {
    const std::string* itemName = items.getById(id);
    if (itemName == nullptr) {
        return std::format("Item<{}>", id);
    } else {
        return *itemName;
    }
}

std::string GameData::getLocationName(location_id_t id) {
    const std::string* locationName = locations.getById(id);
    if (locationName == nullptr) {
        return std::format("Location<{}>", id);
    }
    else {
        return *locationName;
    }
}

std::vector<std::string> GameData::getItemNames() {
    return items.getObjects();
}

std::vector<std::string> GameData::getLocationNames() {
    return locations.getObjects();
}

}