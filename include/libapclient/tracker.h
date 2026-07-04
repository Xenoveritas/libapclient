/*! \file tracker.h
 * \brief Utilities for tracking checked locations and received items.
 *
 * These utilities are useful both for writing a tracker, but also for writing a
 * game client that needs to know what items have been received.
 */

#include <map>
#include <vector>
#include <unordered_map>

#include "packets.h"

namespace archipelago {

/*! \brief Tracks local locations that have been checked.
 *
 * This is a template, allowing the metadata stored for a given location to be
 * whatever an implementation requires.
 *
 * \tparam L the data to store per location. The locations should support a
 *         operator=(bool&) which sets if the location has or has not been
 *         checked. (This allows bool to be used for location data if a tracker
 *         has no additional metadata to associate with a location.)
 * \tparam Allocator the allocator the underlying map uses.
 */
template<class L, class Allocator = std::allocator<std::pair<const location_id_t, L>>> class LocationTracker {
protected:
    /*! \brief A map of location IDs to metadata about those locations.
     *
     * Location IDs need not be sequential, and in fact, frequently aren't.
     * (The location ID is frequently the address of the item in the game's
     * memory.)
     */
    std::map<location_id_t, L, std::less<location_id_t>, Allocator> m_locations;

public:
    /*! \brief Clear all the existing location data.
     */
    void clear() {
        m_locations.clear();
    }

    /*! \brief Copy in the connected packet's information.
     *
     * The Connected packet gives a full accounting of locations that the server
     * is aware have been checked and locations that have not been. This
     * provides the client with a complete list of locations in the game - some
     * games may have locations that aren't included in all game types.
     *
     * \param connected the Connected packet
     * \return itself
     */
    LocationTracker& setFromConnected(const packets::Connected& connected) {
        setLocations(connected.missing_locations, false);
        setLocations(connected.checked_locations, true);
        return *this;
    }

    /*! \brief Copy in the connected packet's information. This is the same as
     * setFromConnected(const packets::Connected&).
     *
     * \param connected the Connected packet
     * \return itself
     */
    LocationTracker& operator<<(const packets::Connected& connected) {
        return setFromConnected(connected);
    }

    /*! \brief Set the given set of locations to the given checked status.
     *
     * \param locationIds the list of location IDs to set
     * \param checked true if checked, false otherwise
     */
    void setLocations(std::vector<location_id_t> locationIds, bool checked) {
        for (location_id_t locId : locationIds) {
            m_locations[locId] = checked;
        }
    }

    /*! \brief Get the location data for a given location.
     *
     * \param location the location ID to look up
     * \return a reference to the location data
     * \throws std::out_of_range if the given location does not exist
     */
    L& getLocation(location_id_t location) {
        return m_locations.at(location);
    }

    /*! \brief Get the location data for a given location.
     *
     * \param location the location ID to look up
     * \return a reference to the location data
     * \throws std::out_of_range if the given location does not exist
     */
    const L& getLocation(location_id_t location) const {
        return m_locations.at(location);
    }

    /*! \brief Get all known location data.
     *
     * \return the set of locations
     */
    std::vector<L> getLocations() {
        std::vector<L> result;
        result.reserve(m_locations.size());
        for (auto it = m_locations.cbegin(); it != m_locations.cend(); ++it) {
            result.push_back(it->second);
        }
        return result;
    }

    /*! \brief Get all known location data.
     *
     * \return the set of locations
     */
    std::vector<location_id_t> getLocationIds() {
        std::vector<location_id_t> result;
        result.reserve(m_locations.size());
        for (auto it = m_locations.cbegin(); it != m_locations.cend(); ++it) {
            result.push_back(it->first);
        }
        return result;
    }
};

/*! \brief Track received items.
 *
 * This mostly provides convenience methods for pulling data from the various
 * packets that provide received item data.
 *
 * \tparam I The class that holds the item info.
 * \tparam Allocator the allocator the underlying vector uses.
 */
template<class I = packets::NetworkItem, class Allocator = std::allocator<I>> class ItemTracker {
public:
    /*! \brief The index of the last item given to the player. */
    size_t index;
    /*! \brief The list of items that the client has received.
     *
     * Archipelago keeps an ordered list of the item IDs that a client has
     * received.
     */
    std::vector<I, Allocator> items;

    /*! \brief Create a new item tracker.
     */
    ItemTracker() : index(0), items() {}
    /*! \brief Create a new item tracker with the given index.
     */
    ItemTracker(size_t aIndex) : index(aIndex), items() {}

    ItemTracker& operator<<(const packets::ReceivedItems& receivedItems) {
        return operator+=(receivedItems.items);
    }
    ItemTracker& operator+=(const std::vector<packets::NetworkItem>& newItems) {
        // Preemptively prepare for this
        items.reserve(items.size() + newItems.size());
        for (auto& item : newItems) {
            items.push_back(I(item));
        }
        return *this;
    }
};

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

}