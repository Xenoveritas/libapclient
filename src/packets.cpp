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
#include "libapclient/logger.h"

namespace archipelago {
namespace packets {


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

const NetworkSlot* SlotInfo::getNetworkSlotByName(const std::string& name) const {
    // can't find, but can:
    for (auto& entry : slot_info) {
        if (entry.second.name == name) {
            return &entry.second;
        }
    }
    return nullptr;
}

const NetworkPlayer* Connected::getPlayer(const std::string& name) const {
    for (auto& player : players) {
        if (player.name == name) {
            return &player;
        }
    }
    return nullptr;
}

bool NetworkVersion::operator==(const NetworkVersion& other) const {
    return major == other.major && minor == other.minor && build == other.build;
}

bool NetworkVersion::operator<(const NetworkVersion& other) const {
    if (major < other.major) {
        return true;
    }
    if (major > other.major) {
        return false;
    }
    // Otherwise major == other.major so move to minor
    if (minor < other.minor) {
        return true;
    }
    if (minor > other.minor) {
        return false;
    }
    // Otherwise minor == other.minor so move to build
    // build == other.build means not less than so that's false
    if (build >= other.build) {
        return false;
    }
    return true;
}

// Conversion functions. The library-provided macros don't *quite* do the trick
// due to some requirements imposed by the Python code this is interacting with.

// These macros mostly exist for typo/copy-paste protection (since the JSON
// field and the C++ field are the same).
/// \cond
// Macro expansion is ... anyway, this allows std::map<foo COMMA bar> in type fields
#define COMMA ,
#define READ_FIELD(json_object, object, field) json_object.at(#field).get_to(object.field)
#define READ_OPTIONAL_FIELD(json_object, object, type, field) { \
    auto _iter = json_object.find(#field); \
    if (_iter != json_object.end()) { \
        type _v = *_iter; \
        object.field = _v; \
    } \
}
#define READ_FIELD_IF_EXISTS(json_object, object, field, blank) { \
    auto _iter = json_object.find(#field); \
    if (_iter == json_object.end()) { \
        object.field = blank; \
    } else { \
        object.field = (*_iter); \
    } \
}

#define WRITE_FIELD(object, field) { #field, object.field }
#define ADD_FIELD(json_object, object, field) json_object[#field] = object.field
#define ADD_FIELD_IF_EXISTS(json_object, object, field) if (object.field.has_value()) { json_object[#field] = (*object.field); }

#define OBJ_WRITE_FIELD(field) { #field, field }
#define OBJ_ADD_FIELD(json_object, field) json_object[#field] = field
#define OBJ_ADD_FIELD_IF_EXISTS(json_object, field) if (field.has_value()) { json_object[#field] = (*field); }

#define DEFER_TO_CLASS(cls) void to_json(json& j, const cls& obj) { obj.to_json(j); }

// CONVERSION FUNCTIONS
//
// These are alphabetized to keep the order somewhat understandable.
void from_json(const json& j, Bounce& bounce) {
    READ_OPTIONAL_FIELD(j, bounce, std::vector<std::string>, games);
    READ_OPTIONAL_FIELD(j, bounce, std::vector<player_id_t>, slots);
    READ_OPTIONAL_FIELD(j, bounce, std::vector<std::string>, tags);
    READ_OPTIONAL_FIELD(j, bounce, json, data);
}

void Bounce::convert_to_json(json& j) const {
    OBJ_ADD_FIELD_IF_EXISTS(j, games);
    OBJ_ADD_FIELD_IF_EXISTS(j, slots);
    OBJ_ADD_FIELD_IF_EXISTS(j, tags);
    OBJ_ADD_FIELD_IF_EXISTS(j, data);
}

DEFER_TO_CLASS(Bounce);

void from_json(const json& j, Bounced& bounced) {
    READ_OPTIONAL_FIELD(j, bounced, std::vector<std::string>, games);
    READ_OPTIONAL_FIELD(j, bounced, std::vector<player_id_t>, slots);
    READ_OPTIONAL_FIELD(j, bounced, std::vector<std::string>, tags);
    READ_OPTIONAL_FIELD(j, bounced, json, data);
}

void Bounced::convert_to_json(json& j) const {
    OBJ_ADD_FIELD_IF_EXISTS(j, games);
    OBJ_ADD_FIELD_IF_EXISTS(j, slots);
    OBJ_ADD_FIELD_IF_EXISTS(j, tags);
    OBJ_ADD_FIELD_IF_EXISTS(j, data);
}

DEFER_TO_CLASS(Bounced);

void from_json(const json& j, Connect& connect) {
    READ_FIELD(j, connect, name);
    // game and password are "optional" in that they're required but the value may sometimes be null.
    auto game = j.at("game");
    if (game.is_null()) {
        connect.game = std::nullopt;
    } else {
        connect.game = game;
    }
    auto password = j.at("password");
    if (password.is_null()) {
        connect.password = std::nullopt;
    } else {
        connect.password = password;
    }
    READ_FIELD(j, connect, uuid);
    READ_FIELD(j, connect, version);
    READ_FIELD(j, connect, items_handling);
    READ_FIELD(j, connect, tags);
    READ_FIELD(j, connect, slot_data);
}

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

DEFER_TO_CLASS(Connect);

void from_json(const json& j, Connected& connected) {
    READ_FIELD(j, connected, team);
    READ_FIELD(j, connected, slot);
    READ_FIELD(j, connected, players);
    READ_FIELD(j, connected, missing_locations);
    READ_FIELD(j, connected, checked_locations);
    // Slot data can be entirely missing
    READ_OPTIONAL_FIELD(j, connected, json, slot_data);
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

DEFER_TO_CLASS(Connected);

ConnectionRefused::ConnectionRefused(const json& jsonData) : Packet(kPacketConnectionRefused), errors() {
    errors = jsonData.at("errors");
}

void from_json(const json& j, ConnectionRefused& connectionRefused) {
    READ_FIELD(j, connectionRefused, errors);
}

void ConnectionRefused::convert_to_json(json& j) const {
    j = json{
        OBJ_WRITE_FIELD(errors)
    };
}

DEFER_TO_CLASS(ConnectionRefused);

void from_json(const json& j, ConnectUpdate& connectUpdate) {
    READ_OPTIONAL_FIELD(j, connectUpdate, ItemsHandling, items_handling);
    READ_OPTIONAL_FIELD(j, connectUpdate, std::vector<std::string>, tags);
}

void ConnectUpdate::convert_to_json(json& j) const {
    j = json::object();
    OBJ_ADD_FIELD_IF_EXISTS(j, items_handling);
    OBJ_ADD_FIELD_IF_EXISTS(j, tags);
}

DEFER_TO_CLASS(ConnectUpdate);

void from_json(const json& j, CreateHints& createHints) {
    READ_FIELD(j, createHints, locations);
    READ_FIELD(j, createHints, player);
    READ_OPTIONAL_FIELD(j, createHints, HintStatus, status);
}

void CreateHints::convert_to_json(json& j) const {
    j = json{
        OBJ_WRITE_FIELD(locations),
        OBJ_WRITE_FIELD(player)
    };
    OBJ_ADD_FIELD_IF_EXISTS(j, status);
}

DEFER_TO_CLASS(CreateHints);

void from_json(const json& j, DataPackage& dataPackage) {
    // This is something like { "data": { "games": (actual JSON) } }
    // Rather than mirror the single-key-object, just extract the games part
    j.at("data").at("games").get_to(dataPackage.games);
}

void DataPackage::convert_to_json(json& j) const {
    j = json{
        { "data", {
            { "games", games }
        } }
    };
}

DEFER_TO_CLASS(DataPackage);

void from_json(const json& j, DataStorageOperation& dataStorageOperation) {
    READ_FIELD(j, dataStorageOperation, operation);
    READ_FIELD(j, dataStorageOperation, value);
}

void to_json(json& j, const DataStorageOperation& dataStorageOperation) {
    j = {
        WRITE_FIELD(dataStorageOperation, operation),
        WRITE_FIELD(dataStorageOperation, value)
    };
}

void from_json(const json& j, DataStorageOperationType& dataStorageOperationType) {
    // Operation is actually a string
    const std::string& jsonType = static_cast<const std::string&>(j);
    if (jsonType == "replace") {
        dataStorageOperationType = DataStorageOperationType::opReplace;
    } else if (jsonType == "default") {
        dataStorageOperationType = DataStorageOperationType::opDefault;
    } else if (jsonType == "add") {
        dataStorageOperationType = DataStorageOperationType::opAdd;
    } else if (jsonType == "mul") {
        dataStorageOperationType = DataStorageOperationType::opMul;
    } else if (jsonType == "pow") {
        dataStorageOperationType = DataStorageOperationType::opPow;
    } else if (jsonType == "mod") {
        dataStorageOperationType = DataStorageOperationType::opMod;
    } else if (jsonType == "floor") {
        dataStorageOperationType = DataStorageOperationType::opFloor;
    } else if (jsonType == "ceil") {
        dataStorageOperationType = DataStorageOperationType::opCeil;
    } else if (jsonType == "max") {
        dataStorageOperationType = DataStorageOperationType::opMax;
    } else if (jsonType == "min") {
        dataStorageOperationType = DataStorageOperationType::opMin;
    } else if (jsonType == "and") {
        dataStorageOperationType = DataStorageOperationType::opAnd;
    } else if (jsonType == "or") {
        dataStorageOperationType = DataStorageOperationType::opOr;
    } else if (jsonType == "xor") {
        dataStorageOperationType = DataStorageOperationType::opXor;
    } else if (jsonType == "leftShift") {
        dataStorageOperationType = DataStorageOperationType::opLeftShift;
    } else if (jsonType == "rightShift") {
        dataStorageOperationType = DataStorageOperationType::opRightShift;
    } else if (jsonType == "remove") {
        dataStorageOperationType = DataStorageOperationType::opRemove;
    } else if (jsonType == "pop") {
        dataStorageOperationType = DataStorageOperationType::opPop;
    } else if (jsonType == "update") {
        dataStorageOperationType = DataStorageOperationType::opUpdate;
    }
}

void to_json(json& j, const DataStorageOperationType& dataStorageOperationType) {
    switch (dataStorageOperationType) {
    case DataStorageOperationType::opReplace:
        j = "replace";
        break;
    case DataStorageOperationType::opDefault:
        j = "default";
        break;
    case DataStorageOperationType::opAdd:
        j = "add";
        break;
    case DataStorageOperationType::opMul:
        j = "mul";
        break;
    case DataStorageOperationType::opPow:
        j = "pow";
        break;
    case DataStorageOperationType::opMod:
        j = "mod";
        break;
    case DataStorageOperationType::opFloor:
        j = "floor";
        break;
    case DataStorageOperationType::opCeil:
        j = "ceil";
        break;
    case DataStorageOperationType::opMax:
        j = "max";
        break;
    case DataStorageOperationType::opMin:
        j = "min";
        break;
    case DataStorageOperationType::opAnd:
        j = "and";
        break;
    case DataStorageOperationType::opOr:
        j = "or";
        break;
    case DataStorageOperationType::opXor:
        j = "xor";
        break;
    case DataStorageOperationType::opLeftShift:
        j = "leftShift";
        break;
    case DataStorageOperationType::opRightShift:
        j = "rightShift";
        break;
    case DataStorageOperationType::opRemove:
        j = "remove";
        break;
    case DataStorageOperationType::opPop:
        j = "pop";
        break;
    case DataStorageOperationType::opUpdate:
        j = "update";
        break;
    default:
        // something is wrong in this case
        j = nullptr;
    }
}

void from_json(const json& j, Get& get) {
    READ_FIELD(j, get, keys);
}

void Get::convert_to_json(json& j) const {
    j = {
        OBJ_WRITE_FIELD(keys)
    };
}

DEFER_TO_CLASS(Get);

void from_json(const json& j, GetDataPackage& getDataPackage) {
    READ_OPTIONAL_FIELD(j, getDataPackage, std::vector<std::string>, games);
}

void GetDataPackage::convert_to_json(json& j) const {
    // This really could be an (almost) empty object
    j = json::object();
    // Only add the games field if one was provided, otherwise, leave it out
    OBJ_ADD_FIELD_IF_EXISTS(j, games);
}

DEFER_TO_CLASS(GetDataPackage);

void from_json(const json& j, InvalidPacket& invalidPacket) {
    READ_FIELD(j, invalidPacket, type);
    READ_OPTIONAL_FIELD(j, invalidPacket, std::string, original_cmd);
    READ_FIELD(j, invalidPacket, text);
}

void InvalidPacket::convert_to_json(json& j) const {
    j = json{
        OBJ_WRITE_FIELD(type),
        OBJ_WRITE_FIELD(text)
    };
    OBJ_ADD_FIELD_IF_EXISTS(j, original_cmd);
}

DEFER_TO_CLASS(InvalidPacket);

void from_json(const json& j, JSONMessagePart& jsonMessagePart) {
    // Everything in this object is optional
    READ_OPTIONAL_FIELD(j, jsonMessagePart, std::string, type);
    READ_OPTIONAL_FIELD(j, jsonMessagePart, std::string, text);
    READ_OPTIONAL_FIELD(j, jsonMessagePart, std::string, color);
    READ_OPTIONAL_FIELD(j, jsonMessagePart, int, flags);
    READ_OPTIONAL_FIELD(j, jsonMessagePart, player_id_t, player);
    READ_OPTIONAL_FIELD(j, jsonMessagePart, HintStatus, hint_status);
}

void to_json(json& j, const JSONMessagePart& jsonMessagePart) {
    j = json::object();
    ADD_FIELD_IF_EXISTS(j, jsonMessagePart, type);
    ADD_FIELD_IF_EXISTS(j, jsonMessagePart, text);
    ADD_FIELD_IF_EXISTS(j, jsonMessagePart, color);
    ADD_FIELD_IF_EXISTS(j, jsonMessagePart, flags);
    ADD_FIELD_IF_EXISTS(j, jsonMessagePart, player);
    ADD_FIELD_IF_EXISTS(j, jsonMessagePart, hint_status);
}

void from_json(const json& j, LocationChecks& locationChecks) {
    READ_FIELD(j, locationChecks, locations);
}

void LocationChecks::convert_to_json(json& j) const {
    j = json{
        OBJ_WRITE_FIELD(locations)
    };
}

DEFER_TO_CLASS(LocationChecks);

void from_json(const json& j, LocationInfo& locationInfo) {
    READ_FIELD(j, locationInfo, locations);
}

void LocationInfo::convert_to_json(json& j) const {
    j = json{
        OBJ_WRITE_FIELD(locations)
    };
}

DEFER_TO_CLASS(LocationInfo);

void from_json(const json& j, LocationScouts& locationScouts) {
    READ_FIELD(j, locationScouts, locations);
    READ_FIELD(j, locationScouts, create_as_hint);
}

void LocationScouts::convert_to_json(json& j) const {
    j = json{
        OBJ_WRITE_FIELD(locations),
        OBJ_WRITE_FIELD(create_as_hint)
    };
}

DEFER_TO_CLASS(LocationScouts);

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
    READ_FIELD_IF_EXISTS(j, printJSON, type, PrintJsonType::none);
    READ_OPTIONAL_FIELD(j, printJSON, player_id_t, receiving);
    READ_OPTIONAL_FIELD(j, printJSON, NetworkItem, item);
    READ_OPTIONAL_FIELD(j, printJSON, bool, found);
    READ_OPTIONAL_FIELD(j, printJSON, team_id_t, team);
    READ_OPTIONAL_FIELD(j, printJSON, team_slot_id_t, slot);
    READ_OPTIONAL_FIELD(j, printJSON, std::string, message);
    READ_OPTIONAL_FIELD(j, printJSON, std::vector<std::string>, tags);
    READ_OPTIONAL_FIELD(j, printJSON, int, countdown);
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

DEFER_TO_CLASS(ReceivedItems);

void from_json(const json& j, Retrieved& retrieved) {
    READ_FIELD(j, retrieved, keys);
}

void Retrieved::convert_to_json(json& j) const {
    j = json{
        OBJ_WRITE_FIELD(keys)
    };
}

DEFER_TO_CLASS(Retrieved);

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

void from_json(const json& j, RoomUpdate& roomUpdate) {
    READ_OPTIONAL_FIELD(j, roomUpdate, int, hint_points);
    READ_OPTIONAL_FIELD(j, roomUpdate, int, location_check_points);
    READ_OPTIONAL_FIELD(j, roomUpdate, std::vector<NetworkPlayer>, players);
    READ_OPTIONAL_FIELD(j, roomUpdate, std::vector<location_id_t>, checked_locations);
    READ_OPTIONAL_FIELD(j, roomUpdate, std::unordered_map<std::string COMMA Permission>, permissions);
}

void RoomUpdate::convert_to_json(json& j) const {
    OBJ_ADD_FIELD_IF_EXISTS(j, hint_points);
    OBJ_ADD_FIELD_IF_EXISTS(j, location_check_points);
    OBJ_ADD_FIELD_IF_EXISTS(j, players);
    OBJ_ADD_FIELD_IF_EXISTS(j, checked_locations);
    OBJ_ADD_FIELD_IF_EXISTS(j, permissions);
}

DEFER_TO_CLASS(RoomUpdate);

void from_json(const json& j, Say& say) {
    READ_FIELD(j, say, text);
}

void Say::convert_to_json(json& j) const {
    j = json{
        OBJ_WRITE_FIELD(text)
    };
}

DEFER_TO_CLASS(Say);

void from_json(const json& j, Set& set) {
    READ_FIELD(j, set, key);
    j.at("default").get_to(set.default_value);
    READ_FIELD(j, set, want_reply);
    READ_FIELD(j, set, operations);
}

void Set::convert_to_json(json& j) const {
    j = json{
        OBJ_WRITE_FIELD(key),
        { "default", default_value },
        OBJ_WRITE_FIELD(want_reply),
        OBJ_WRITE_FIELD(operations)
    };
}

DEFER_TO_CLASS(Set);

void from_json(const json& j, SetNotify& setNotify) {
    READ_FIELD(j, setNotify, keys);
}

void SetNotify::convert_to_json(json& j) const {
    j = json{
        OBJ_WRITE_FIELD(keys)
    };
}

DEFER_TO_CLASS(SetNotify);

void from_json(const json& j, SetReply& setReply) {
    READ_FIELD(j, setReply, key);
    READ_FIELD(j, setReply, value);
    READ_OPTIONAL_FIELD(j, setReply, json, original_value);
    READ_FIELD(j, setReply, slot);
}

void SetReply::convert_to_json(json& j) const {
    j = json{
        OBJ_WRITE_FIELD(key),
        OBJ_WRITE_FIELD(value),
        OBJ_WRITE_FIELD(slot)
    };
    OBJ_ADD_FIELD_IF_EXISTS(j, original_value);
}

DEFER_TO_CLASS(SetReply);

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

DEFER_TO_CLASS(StatusUpdate);

void from_json(const json& j, Sync& sync) {
    // There's no payload to decode, so this really is a no-op
}

void Sync::convert_to_json(json& j) const {
    j = json::object();
}

DEFER_TO_CLASS(Sync);

void from_json(const json& j, UpdateHint& updateHint) {
    READ_FIELD(j, updateHint, player);
    READ_FIELD(j, updateHint, location);
    READ_OPTIONAL_FIELD(j, updateHint, HintStatus, status);
}

void UpdateHint::convert_to_json(json& j) const {
    j = json{
        OBJ_WRITE_FIELD(player),
        OBJ_WRITE_FIELD(location)
    };
    OBJ_ADD_FIELD_IF_EXISTS(j, status);
}

DEFER_TO_CLASS(UpdateHint);
/// \endcond

} // namespace packets
} // namespace archipelago