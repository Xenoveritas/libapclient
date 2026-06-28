/*! \file packets.cpp
 * \brief implementation of packet helpers.
 *
 * Most of this file consists of helper macros and functions that exist to
 * allow conversion via typecasts through the
 * [nlohmann JSON parser](https://github.com/nlohmann/json). These aren't
 * documented since they aren't meant to be called directly, they're called
 * via the JSON templates in `nlohmann::json`.
 */

#include "libapclient/packets.h"

namespace archipelago {
namespace packets {

// Conversion functions. The library-provided macros don't *quite* do the trick
// due to some requirements imposed by the Python code this is interacting with.

// These macros mostly exist for typo/copy-paste protection (since the JSON
// field and the C++ field are the same).
/// \cond
#define READ_FIELD(json_object, object, field) json_object.at(#field).get_to(object.field)
#define READ_OPTIONAL_FIELD(json_object, object, field) { auto _iter = json_object.find(#field); if (_iter != json_object.end()) { object.field = *_iter; } }

#define WRITE_FIELD(object, field) { #field, object.field }
#define ADD_FIELD(json_object, object, field) json_object[#field] = object.field
#define ADD_FIELD_IF_EXISTS(json_object, object, field) if (object.field.has_value()) { json_object[#field] = (*object.field); }

#define OBJ_WRITE_FIELD(field) { #field, field }
#define OBJ_ADD_FIELD(json_object, field) json_object[#field] = field
#define OBJ_ADD_FIELD_IF_EXISTS(json_object, field) if (field.has_value()) { json_object[#field] = (*field); }

#define DEFER_TO_CLASS(cls) void to_json(json& j, const cls& obj) { obj.to_json(j); }
/// \endcond
// Packet helper implementations

bool ConnectionRefused::has_error(const std::string& error) const {
    // Check to see if the error is in the error list. This takes o(n) time -
    // but given that the error list will be at most five elements (if you
    // manage to hit every error Archipelago can return) this is still likely
    // faster than trying to "optimize" a lookup.
    for (auto it = errors.cbegin(); it != errors.cend(); ++it) {
        if (error == *it) {
            return true;
        }
    }
    return false;
}

// CONVERSION FUNCTIONS
//
// These are alphabetized to keep the order somewhat understandable.
/// \cond
void Connect::convert_to_json(json& j) const {
    j = json{
        OBJ_WRITE_FIELD(name),
        // game and password are "optional" in that they're required but the value may sometimes be null.
        OBJ_WRITE_FIELD(game),
        OBJ_WRITE_FIELD(password),
        OBJ_WRITE_FIELD(uuid),
        OBJ_WRITE_FIELD(version),
        OBJ_WRITE_FIELD(items_handling),
        OBJ_WRITE_FIELD(tags),
        OBJ_WRITE_FIELD(slot_data)
    };
}

void from_json(const json& j, Connected& connected) {
    READ_FIELD(j, connected, team);
    READ_FIELD(j, connected, slot);
    READ_FIELD(j, connected, players);
    READ_FIELD(j, connected, missing_locations);
    READ_FIELD(j, connected, checked_locations);
    // Slot data can be entirely missing
    READ_OPTIONAL_FIELD(j, connected, slot_data);
    READ_FIELD(j, connected, slot_info);
    READ_FIELD(j, connected, hint_points);
}

void Connected::convert_to_json(json& j) const {
    j = json{
        OBJ_WRITE_FIELD(team),
        OBJ_WRITE_FIELD(slot),
        OBJ_WRITE_FIELD(players),
        OBJ_WRITE_FIELD(missing_locations),
        OBJ_WRITE_FIELD(checked_locations),
        OBJ_WRITE_FIELD(slot_data),
        OBJ_WRITE_FIELD(slot_info),
        OBJ_WRITE_FIELD(hint_points)
    };
}

void from_json(const json& j, ConnectionRefused& connectionRefused) {
    READ_FIELD(j, connectionRefused, errors);
}

void ConnectionRefused::convert_to_json(json& j) const {
    j = json{
        OBJ_WRITE_FIELD(errors)
    };
}

void from_json(const json& j, DataPackage& dataPackage) {
    // This is something like { "data": { "games": (actual JSON) } }
    // Rather than mirror the single-key-object, just extract the games part
    j.at("data").at("games").get_to(dataPackage.games);
}

DEFER_TO_CLASS(DataPackage);

void DataPackage::convert_to_json(json& j) const {
    j = json{
        { "data", {
            { "games", games }
        } }
    };
}

void GetDataPackage::convert_to_json(json& j) const {
    // This really could be an (almost) empty object
    j = json::object();
    // Only add the games field if one was provided, otherwise, leave it out
    OBJ_ADD_FIELD_IF_EXISTS(j, games);
}

void from_json(const json& j, InvalidPacket& invalidPacket) {
    READ_FIELD(j, invalidPacket, type);
    READ_OPTIONAL_FIELD(j, invalidPacket, original_cmd);
    READ_FIELD(j, invalidPacket, text);
}

void InvalidPacket::convert_to_json(json& j) const {
    j = json{
        OBJ_WRITE_FIELD(type),
        OBJ_WRITE_FIELD(text)
    };
    OBJ_ADD_FIELD_IF_EXISTS(j, original_cmd);
}

void from_json(const json& j, JSONMessagePart& jsonMessagePart) {
    // Everything in this object is optional
    READ_OPTIONAL_FIELD(j, jsonMessagePart, type);
    READ_OPTIONAL_FIELD(j, jsonMessagePart, text);
    READ_OPTIONAL_FIELD(j, jsonMessagePart, color);
    READ_OPTIONAL_FIELD(j, jsonMessagePart, flags);
    READ_OPTIONAL_FIELD(j, jsonMessagePart, player);
    // TODO: READ_OPTIONAL_FIELD(j, jsonMessagePart, hint_status);
}

void to_json(json& j, const JSONMessagePart& jsonMessagePart) {
    j = json::object();
    ADD_FIELD_IF_EXISTS(j, jsonMessagePart, type);
    ADD_FIELD_IF_EXISTS(j, jsonMessagePart, text);
    ADD_FIELD_IF_EXISTS(j, jsonMessagePart, color);
    ADD_FIELD_IF_EXISTS(j, jsonMessagePart, flags);
    ADD_FIELD_IF_EXISTS(j, jsonMessagePart, player);
}

void from_json(const json& j, LocationChecks& locationChecks) {
    j.at("locations").get_to(locationChecks.locations);
}

void LocationChecks::convert_to_json(json& j) const {
    j = json{
        OBJ_WRITE_FIELD(locations)
    };
}

void from_json(const json& j, NetworkItem& item) {
    READ_FIELD(j, item, item);
    READ_FIELD(j, item, location);
    READ_FIELD(j, item, player);
    READ_FIELD(j, item, flags);
}

void to_json(json& j, const NetworkItem& item) {
    j = json{
        // class is required for Python
        { "class", "NetworkItem" },
        WRITE_FIELD(item, item),
        WRITE_FIELD(item, location),
        WRITE_FIELD(item, player),
        WRITE_FIELD(item, flags)
    };
}

void from_json(const json& j, NetworkPlayer& player) {
    READ_FIELD(j, player, team);
    READ_FIELD(j, player, slot);
    READ_FIELD(j, player, alias);
    READ_FIELD(j, player, name);
}

void to_json(json& j, const NetworkPlayer& player) {
    j = json{
        // class is required for Python
        { "class", "NetworkPlayer" },
        { "team", player.team },
        { "slot", player.slot },
        { "alias", player.alias },
        { "name", player.name }
    };
}

void from_json(const json& j, NetworkSlot& slot) {
    j.at("name").get_to(slot.name);
    j.at("game").get_to(slot.game);
    j.at("type").get_to(slot.type);
    j.at("group_members").get_to(slot.group_members);
}

void to_json(json& j, const NetworkSlot& slot) {
    j = json{
        // class is required for Python
        { "class", "NetworkSlot" },
        { "name", slot.name },
        { "game", slot.game },
        { "type", slot.type },
        { "group_members", slot.group_members }
    };
}

void from_json(const json& j, NetworkVersion& version) {
    READ_FIELD(j, version, major);
    READ_FIELD(j, version, minor);
    READ_FIELD(j, version, build);
}

void to_json(json& j, const NetworkVersion& version) {
    j = json{
        // class is required for Python
        { "class", "Version" },
        WRITE_FIELD(version, major),
        WRITE_FIELD(version, minor),
        WRITE_FIELD(version, build)
    };
}

void from_json(const json& j, PrintJSON& printJSON) {
    READ_FIELD(j, printJSON, data);
    READ_OPTIONAL_FIELD(j, printJSON, type);
    READ_OPTIONAL_FIELD(j, printJSON, receiving);
    READ_OPTIONAL_FIELD(j, printJSON, item);
    READ_OPTIONAL_FIELD(j, printJSON, found);
    READ_OPTIONAL_FIELD(j, printJSON, team);
    READ_OPTIONAL_FIELD(j, printJSON, slot);
    READ_OPTIONAL_FIELD(j, printJSON, message);
    // Tags doesn't quite work with the macro
    auto tagsIt = j.find("tags");
    if (tagsIt != j.end()) {
        std::vector<std::string> tags = *tagsIt;
        printJSON.tags = tags;
    }
    READ_OPTIONAL_FIELD(j, printJSON, countdown);
}

DEFER_TO_CLASS(PrintJSON);

void PrintJSON::convert_to_json(json& j) const {
    j = json{
        OBJ_WRITE_FIELD(data)
    };
    // We handle type in a weird way
    if (type != PrintJsonType::none && type != PrintJsonType::unknown) {
        OBJ_ADD_FIELD(j, type);
    }
    OBJ_ADD_FIELD_IF_EXISTS(j, receiving);
    OBJ_ADD_FIELD_IF_EXISTS(j, item);
    OBJ_ADD_FIELD_IF_EXISTS(j, found);
    OBJ_ADD_FIELD_IF_EXISTS(j, team);
    OBJ_ADD_FIELD_IF_EXISTS(j, slot);
    OBJ_ADD_FIELD_IF_EXISTS(j, message);
}

void from_json(const json& j, PrintJsonType& printJsonType) {
    if (j.is_null() || j.is_discarded()) {
        printJsonType = PrintJsonType::none;
        return;
    }
    // This is a weird one: the type should actually be a string.
    const std::string& jsonType = static_cast<const std::string&>(j);
    if (jsonType == "ItemSend") {
        printJsonType = PrintJsonType::ItemSend;
    } else if (jsonType == "ItemCheat") {
        printJsonType = PrintJsonType::ItemCheat;
    } else if (jsonType == "Hint") {
        printJsonType = PrintJsonType::Hint;
    } else if (jsonType == "Join") {
        printJsonType = PrintJsonType::Join;
    } else if (jsonType == "Part") {
        printJsonType = PrintJsonType::Part;
    } else if (jsonType == "Chat") {
        printJsonType = PrintJsonType::Chat;
    } else if (jsonType == "ServerChat") {
        printJsonType = PrintJsonType::ServerChat;
    } else if (jsonType == "Tutorial") {
        printJsonType = PrintJsonType::Tutorial;
    } else if (jsonType == "TagsChanged") {
        printJsonType = PrintJsonType::TagsChanged;
    } else if (jsonType == "CommandResult") {
        printJsonType = PrintJsonType::CommandResult;
    } else if (jsonType == "AdminCommandResult") {
        printJsonType = PrintJsonType::AdminCommandResult;
    } else if (jsonType == "Goal") {
        printJsonType = PrintJsonType::Goal;
    } else if (jsonType == "Release") {
        printJsonType = PrintJsonType::Release;
    } else if (jsonType == "Collect") {
        printJsonType = PrintJsonType::Collect;
    } else if (jsonType == "Countdown") {
        printJsonType = PrintJsonType::Countdown;
    } else {
        printJsonType = PrintJsonType::unknown;
    }
}

void to_json(json& j, const PrintJsonType& printJsonType) {
    switch (printJsonType) {
    case PrintJsonType::ItemSend:
        j = "ItemSend";
        break;
    case PrintJsonType::ItemCheat:
        j = "ItemCheat";
        break;
    case PrintJsonType::Hint:
        j = "Hint";
        break;
    case PrintJsonType::Join:
        j = "Join";
        break;
    case PrintJsonType::Part:
        j = "Part";
        break;
    case PrintJsonType::Chat:
        j = "Chat";
        break;
    case PrintJsonType::ServerChat:
        j = "ServerChat";
        break;
    case PrintJsonType::Tutorial:
        j = "Tutorial";
        break;
    case PrintJsonType::TagsChanged:
        j = "TagsChanged";
        break;
    case PrintJsonType::CommandResult:
        j = "CommandResult";
        break;
    case PrintJsonType::AdminCommandResult:
        j = "AdminCommandResult";
        break;
    case PrintJsonType::Goal:
        j = "Goal";
        break;
    case PrintJsonType::Release:
        j = "Release";
        break;
    case PrintJsonType::Collect:
        j = "Collect";
        break;
    case PrintJsonType::Countdown:
        j = "Countdown";
        break;
    case PrintJsonType::none:
    case PrintJsonType::unknown:
    default:
        j = nullptr;
        break;
    }
}

void from_json(const json& j, ReceivedItems& receivedItems) {
    READ_FIELD(j, receivedItems, index);
    READ_FIELD(j, receivedItems, items);
}

void ReceivedItems::convert_to_json(json& j) const {
    j = json{
        OBJ_WRITE_FIELD(index),
        OBJ_WRITE_FIELD(items)
    };
}

void RoomInfo::convert_to_json(json& j) const {
    j = json{
        OBJ_WRITE_FIELD(version),
        OBJ_WRITE_FIELD(generator_version),
        OBJ_WRITE_FIELD(tags),
        OBJ_WRITE_FIELD(password),
        OBJ_WRITE_FIELD(permissions),
        OBJ_WRITE_FIELD(hint_cost),
        OBJ_WRITE_FIELD(location_check_points),
        OBJ_WRITE_FIELD(games),
        OBJ_WRITE_FIELD(datapackage_checksums),
        OBJ_WRITE_FIELD(seed_name),
        OBJ_WRITE_FIELD(time)
    };
}

DEFER_TO_CLASS(RoomInfo);

void from_json(const json& j, RoomInfo& roomInfo) {
    READ_FIELD(j, roomInfo, version);
    READ_FIELD(j, roomInfo, generator_version);
    READ_FIELD(j, roomInfo, tags);
    READ_FIELD(j, roomInfo, password);
    READ_FIELD(j, roomInfo, permissions);
    READ_FIELD(j, roomInfo, hint_cost);
    READ_FIELD(j, roomInfo, location_check_points);
    READ_FIELD(j, roomInfo, games);
    READ_FIELD(j, roomInfo, datapackage_checksums);
    READ_FIELD(j, roomInfo, seed_name);
    READ_FIELD(j, roomInfo, time);
}

void from_json(const json& j, Say& say) {
    READ_FIELD(j, say, text);
}

void Say::convert_to_json(json& j) const {
    j = json{
        OBJ_WRITE_FIELD(text)
    };
}

void to_json(json& j, const SlotInfo& slotInfo) {
    // Start with a blank object.
    j = json::object();
    for (auto& item : slotInfo.slot_info) {
        j[std::to_string(item.first)] = item.second;
    }
}

void from_json(const json& j, SlotInfo& slotInfo) {
    // This is pretty much the opposite of the above.
    slotInfo.slot_info.clear();
    for (auto& item : j.items()) {
        slotInfo.slot_info.insert({ std::stoi(item.key()), item.value() });
    }
}

void from_json(const json& j, StatusUpdate& statusUpdate) {
    READ_FIELD(j, statusUpdate, status);
}

void StatusUpdate::convert_to_json(json& j) const {
    j = json{
        OBJ_WRITE_FIELD(status)
    };
}
/// \endcond

} // namespace packets
} // namespace archipelago