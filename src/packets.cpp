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


#define FROM_JSON(NAME) void from_json(const json& _jsonObject, NAME& _packetStruct)
#define READ_FIELD(FIELD) _jsonObject.at(#FIELD).get_to(_packetStruct.FIELD)
#define READ_NULLABLE_OPTIONAL_FIELD(FIELD) { auto _v = _jsonObject.at(#FIELD); \
    if (_v.is_null()) { \
        _packetStruct.FIELD = std::nullopt; \
    } else { \
        _packetStruct.FIELD = _v; \
    } \
}
#define READ_OPTIONAL_FIELD(TYPE, FIELD) { \
    auto _iter = _jsonObject.find(#FIELD); \
    if (_iter != _jsonObject.end()) { \
        TYPE _v = *_iter; \
        _packetStruct.FIELD = _v; \
    } \
}
#define READ_FIELD_IF_EXISTS(FIELD, BLANK_VALUE) { \
    auto _iter = _jsonObject.find(#FIELD); \
    if (_iter == _jsonObject.end()) { \
        _packetStruct.FIELD = BLANK_VALUE; \
    } else { \
        _packetStruct.FIELD = (*_iter); \
    } \
}


#define TO_JSON(NAME)              void to_json(json& _jsonObject, const NAME& _packetStruct)
#define OBJECT                     _jsonObject = json
#define PACKET(NAME)               { "cmd", kPacket##NAME }
#define PYTHON_CLASS(KLS)          { "class", KLS }
#define WRITE_FIELD(field)         { #field, _packetStruct.field }
#define ADD_FIELD(field)           _jsonObject[#field] = _packetStruct.field
#define ADD_FIELD_IF_EXISTS(field) if (_packetStruct.field.has_value()) { _jsonObject[#field] = (*_packetStruct.field); }


// CONVERSION FUNCTIONS
//
// These are alphabetized to keep the order somewhat understandable.
FROM_JSON(Bounce) {
    READ_OPTIONAL_FIELD(std::vector<std::string>, games);
    READ_OPTIONAL_FIELD(std::vector<player_id_t>, slots);
    READ_OPTIONAL_FIELD(std::vector<std::string>, tags);
    READ_OPTIONAL_FIELD(json, data);
}

TO_JSON(Bounce) {
    OBJECT{
        PACKET(Bounce)
    };
    ADD_FIELD_IF_EXISTS(games);
    ADD_FIELD_IF_EXISTS(slots);
    ADD_FIELD_IF_EXISTS(tags);
    ADD_FIELD_IF_EXISTS(data);
}

FROM_JSON(Bounced) {
    READ_OPTIONAL_FIELD(std::vector<std::string>, games);
    READ_OPTIONAL_FIELD(std::vector<player_id_t>, slots);
    READ_OPTIONAL_FIELD(std::vector<std::string>, tags);
    READ_OPTIONAL_FIELD(json, data);
}

TO_JSON(Bounced) {
    OBJECT{
        PACKET(Bounced)
    };
    ADD_FIELD_IF_EXISTS(games);
    ADD_FIELD_IF_EXISTS(slots);
    ADD_FIELD_IF_EXISTS(tags);
    ADD_FIELD_IF_EXISTS(data);
}

FROM_JSON(Connect) {
    READ_FIELD(name);
    // game and password are "optional" in that they're required but the value may sometimes be null.
    READ_NULLABLE_OPTIONAL_FIELD(game);
    READ_NULLABLE_OPTIONAL_FIELD(password);
    READ_FIELD(uuid);
    READ_FIELD(version);
    READ_FIELD(items_handling);
    READ_FIELD(tags);
    READ_FIELD(slot_data);
}

TO_JSON(Connect) {
    OBJECT{
        PACKET(Connect),
        WRITE_FIELD(name),
        // game and password are "optional" in that they're required but the value may sometimes be null.
        WRITE_FIELD(game),
        WRITE_FIELD(password),
        WRITE_FIELD(uuid),
        WRITE_FIELD(version),
        WRITE_FIELD(items_handling),
        WRITE_FIELD(tags),
        WRITE_FIELD(slot_data)
    };
}

FROM_JSON(Connected) {
    READ_FIELD(team);
    READ_FIELD(slot);
    READ_FIELD(players);
    READ_FIELD(missing_locations);
    READ_FIELD(checked_locations);
    // Slot data can be entirely missing
    READ_OPTIONAL_FIELD(json, slot_data);
    READ_FIELD(slot_info);
    READ_FIELD(hint_points);
}

TO_JSON(Connected) {
    OBJECT{
        PACKET(Connected),
        WRITE_FIELD(team),
        WRITE_FIELD(slot),
        WRITE_FIELD(players),
        WRITE_FIELD(missing_locations),
        WRITE_FIELD(checked_locations),
        WRITE_FIELD(slot_data),
        WRITE_FIELD(slot_info),
        WRITE_FIELD(hint_points)
    };
}

FROM_JSON(ConnectionRefused) {
    READ_FIELD(errors);
}

TO_JSON(ConnectionRefused) {
    OBJECT{
        PACKET(ConnectionRefused),
        WRITE_FIELD(errors)
    };
}

FROM_JSON(ConnectUpdate) {
    READ_OPTIONAL_FIELD(ItemsHandling, items_handling);
    READ_OPTIONAL_FIELD(std::vector<std::string>, tags);
}

TO_JSON(ConnectUpdate) {
    OBJECT{
        PACKET(ConnectUpdate)
    };
    ADD_FIELD_IF_EXISTS(items_handling);
    ADD_FIELD_IF_EXISTS(tags);
}

FROM_JSON(CreateHints) {
    READ_FIELD(locations);
    READ_FIELD(player);
    READ_OPTIONAL_FIELD(HintStatus, status);
}

TO_JSON(CreateHints) {
    OBJECT{
        PACKET(CreateHints),
        WRITE_FIELD(locations),
        WRITE_FIELD(player)
    };
    ADD_FIELD_IF_EXISTS(status);
}

FROM_JSON(DataPackage) {
    // This is something like { "data": { "games": (actual JSON) } }
    // Rather than mirror the single-key-object, just extract the games part
    _jsonObject.at("data").at("games").get_to(_packetStruct.games);
}

TO_JSON(DataPackage) {
    OBJECT{
        PACKET(DataPackage),
        { "data", {
            WRITE_FIELD(games)
        } }
    };
}

FROM_JSON(DataStorageOperation) {
    READ_FIELD(operation);
    READ_FIELD(value);
}

TO_JSON(DataStorageOperation) {
    OBJECT{
        WRITE_FIELD(operation),
        WRITE_FIELD(value)
    };
}

// Don't bother with the macros for this one, it's converting from a string to an enumerated type
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

// See above
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

FROM_JSON(Get) {
    READ_FIELD(keys);
}

TO_JSON(Get) {
    OBJECT{
        PACKET(Get),
        WRITE_FIELD(keys)
    };
}

FROM_JSON(GetDataPackage) {
    READ_OPTIONAL_FIELD(std::vector<std::string>, games);
}

TO_JSON(GetDataPackage) {
    OBJECT{
        PACKET(GetDataPackage)
    };
    // Only add the games field if one was provided, otherwise, leave it out
    ADD_FIELD_IF_EXISTS(games);
}

FROM_JSON(InvalidPacket) {
    READ_FIELD(type);
    READ_NULLABLE_OPTIONAL_FIELD(original_cmd);
    READ_FIELD(text);
}

TO_JSON(InvalidPacket) {
    OBJECT{
        PACKET(InvalidPacket),
        WRITE_FIELD(type),
        WRITE_FIELD(text),
        WRITE_FIELD(original_cmd)
    };
}

FROM_JSON(JSONMessagePart) {
    // Everything in this object is optional
    READ_OPTIONAL_FIELD(std::string, type);
    READ_OPTIONAL_FIELD(std::string, text);
    READ_OPTIONAL_FIELD(std::string, color);
    READ_OPTIONAL_FIELD(int, flags);
    READ_OPTIONAL_FIELD(player_id_t, player);
    READ_OPTIONAL_FIELD(HintStatus, hint_status);
}

TO_JSON(JSONMessagePart) {
    OBJECT{};
    ADD_FIELD_IF_EXISTS(type);
    ADD_FIELD_IF_EXISTS(text);
    ADD_FIELD_IF_EXISTS(color);
    ADD_FIELD_IF_EXISTS(flags);
    ADD_FIELD_IF_EXISTS(player);
    ADD_FIELD_IF_EXISTS(hint_status);
}

FROM_JSON(LocationChecks) {
    READ_FIELD(locations);
}

TO_JSON(LocationChecks) {
    OBJECT{
        PACKET(LocationChecks),
        WRITE_FIELD(locations)
    };
}

FROM_JSON(LocationInfo) {
    READ_FIELD(locations);
}

TO_JSON(LocationInfo) {
    OBJECT{
        PACKET(LocationInfo),
        WRITE_FIELD(locations)
    };
}

FROM_JSON(LocationScouts) {
    READ_FIELD(locations);
    READ_FIELD(create_as_hint);
}

TO_JSON(LocationScouts) {
    OBJECT{
        PACKET(LocationScouts),
        WRITE_FIELD(locations),
        WRITE_FIELD(create_as_hint)
    };
}

FROM_JSON(NetworkItem) {
    READ_FIELD(item);
    READ_FIELD(location);
    READ_FIELD(player);
    READ_FIELD(flags);
}

TO_JSON(NetworkItem) {
    OBJECT{
        // class is required for Python
        PYTHON_CLASS("NetworkItem"),
        WRITE_FIELD(item),
        WRITE_FIELD(location),
        WRITE_FIELD(player),
        WRITE_FIELD(flags)
    };
}

FROM_JSON(NetworkPlayer) {
    READ_FIELD(team);
    READ_FIELD(slot);
    READ_FIELD(alias);
    READ_FIELD(name);
}

TO_JSON(NetworkPlayer) {
    OBJECT{
        // class is required for Python
        PYTHON_CLASS("NetworkPlayer"),
        WRITE_FIELD(team),
        WRITE_FIELD(slot),
        WRITE_FIELD(alias),
        WRITE_FIELD(name)
    };
}

FROM_JSON(NetworkSlot) {
    READ_FIELD(name);
    READ_FIELD(game);
    READ_FIELD(type);
    READ_FIELD(group_members);
}

TO_JSON(NetworkSlot) {
    OBJECT{
        // class is required for Python
        PYTHON_CLASS("NetworkSlot"),
        WRITE_FIELD(name),
        WRITE_FIELD(game),
        WRITE_FIELD(type),
        WRITE_FIELD(group_members)
    };
}

FROM_JSON(NetworkVersion) {
    READ_FIELD(major);
    READ_FIELD(minor);
    READ_FIELD(build);
}

TO_JSON(NetworkVersion) {
    OBJECT{
        // class is required for Python
        PYTHON_CLASS("Version"),
        WRITE_FIELD(major),
        WRITE_FIELD(minor),
        WRITE_FIELD(build)
    };
}

FROM_JSON(PrintJSON) {
    READ_FIELD(data);
    READ_FIELD_IF_EXISTS(type, PrintJsonType::none);
    READ_OPTIONAL_FIELD(player_id_t, receiving);
    READ_OPTIONAL_FIELD(NetworkItem, item);
    READ_OPTIONAL_FIELD(bool, found);
    READ_OPTIONAL_FIELD(team_id_t, team);
    READ_OPTIONAL_FIELD(team_slot_id_t, slot);
    READ_OPTIONAL_FIELD(std::string, message);
    READ_OPTIONAL_FIELD(std::vector<std::string>, tags);
    READ_OPTIONAL_FIELD(int, countdown);
}

TO_JSON(PrintJSON) {
    OBJECT{
        PACKET(PrintJSON),
        WRITE_FIELD(data)
    };
    // We handle type in a weird way
    if (_packetStruct.type != PrintJsonType::none && _packetStruct.type != PrintJsonType::unknown) {
        ADD_FIELD(type);
    }
    ADD_FIELD_IF_EXISTS(receiving);
    ADD_FIELD_IF_EXISTS(item);
    ADD_FIELD_IF_EXISTS(found);
    ADD_FIELD_IF_EXISTS(team);
    ADD_FIELD_IF_EXISTS(slot);
    ADD_FIELD_IF_EXISTS(message);
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

FROM_JSON(ReceivedItems) {
    READ_FIELD(index);
    READ_FIELD(items);
}

TO_JSON(ReceivedItems) {
    OBJECT{
        PACKET(ReceivedItems),
        WRITE_FIELD(index),
        WRITE_FIELD(items)
    };
}

FROM_JSON(Retrieved) {
    READ_FIELD(keys);
}

TO_JSON(Retrieved) {
    OBJECT{
        PACKET(Retrieved),
        WRITE_FIELD(keys)
    };
}

FROM_JSON(RoomInfo) {
    READ_FIELD(version);
    READ_FIELD(generator_version);
    READ_FIELD(tags);
    READ_FIELD(password);
    READ_FIELD(permissions);
    READ_FIELD(hint_cost);
    READ_FIELD(location_check_points);
    READ_FIELD(games);
    READ_FIELD(datapackage_checksums);
    READ_FIELD(seed_name);
    READ_FIELD(time);
}

TO_JSON(RoomInfo) {
    OBJECT{
        PACKET(RoomInfo),
        WRITE_FIELD(version),
        WRITE_FIELD(generator_version),
        WRITE_FIELD(tags),
        WRITE_FIELD(password),
        WRITE_FIELD(permissions),
        WRITE_FIELD(hint_cost),
        WRITE_FIELD(location_check_points),
        WRITE_FIELD(games),
        WRITE_FIELD(datapackage_checksums),
        WRITE_FIELD(seed_name),
        WRITE_FIELD(time)
    };
}

FROM_JSON(RoomUpdate) {
    READ_OPTIONAL_FIELD(int, hint_points);
    READ_OPTIONAL_FIELD(int, location_check_points);
    READ_OPTIONAL_FIELD(std::vector<NetworkPlayer>, players);
    READ_OPTIONAL_FIELD(std::vector<location_id_t>, checked_locations);
    READ_OPTIONAL_FIELD(std::map<std::string COMMA Permission>, permissions);
}

TO_JSON(RoomUpdate) {
    OBJECT{
        PACKET(RoomUpdate)
    };
    ADD_FIELD_IF_EXISTS(hint_points);
    ADD_FIELD_IF_EXISTS(location_check_points);
    ADD_FIELD_IF_EXISTS(players);
    ADD_FIELD_IF_EXISTS(checked_locations);
    ADD_FIELD_IF_EXISTS(permissions);
}

FROM_JSON(Say) {
    READ_FIELD(text);
}

TO_JSON(Say) {
    OBJECT{
        PACKET(Say),
        WRITE_FIELD(text)
    };
}

FROM_JSON(Set) {
    READ_FIELD(key);
    _jsonObject.at("default").get_to(_packetStruct.default_value);
    READ_FIELD(want_reply);
    READ_FIELD(operations);
}

TO_JSON(Set) {
    OBJECT{
        PACKET(Set),
        WRITE_FIELD(key),
        { "default", _packetStruct.default_value },
        WRITE_FIELD(want_reply),
        WRITE_FIELD(operations)
    };
}

FROM_JSON(SetNotify) {
    READ_FIELD(keys);
}

TO_JSON(SetNotify) {
    OBJECT{
        PACKET(SetNotify),
        WRITE_FIELD(keys)
    };
}

FROM_JSON(SetReply) {
    READ_FIELD(key);
    READ_FIELD(value);
    READ_OPTIONAL_FIELD(json, original_value);
    READ_FIELD(slot);
}

TO_JSON(SetReply) {
    OBJECT{
        PACKET(SetReply),
        WRITE_FIELD(key),
        WRITE_FIELD(value),
        WRITE_FIELD(slot)
    };
    ADD_FIELD_IF_EXISTS(original_value);
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

FROM_JSON(StatusUpdate) {
    READ_FIELD(status);
}

TO_JSON(StatusUpdate) {
    OBJECT{
        PACKET(StatusUpdate),
        WRITE_FIELD(status)
    };
}

FROM_JSON(Sync) {
    // There's no payload to decode, so this really is a no-op
}

TO_JSON(Sync) {
    OBJECT{ PACKET(Sync) };
}

FROM_JSON(UpdateHint) {
    READ_FIELD(player);
    READ_FIELD(location);
    READ_OPTIONAL_FIELD(HintStatus, status);
}

TO_JSON(UpdateHint) {
    OBJECT{
        PACKET(UpdateHint),
        WRITE_FIELD(player),
        WRITE_FIELD(location)
    };
    ADD_FIELD_IF_EXISTS(status);
}

/// \endcond

} // namespace packets
} // namespace archipelago