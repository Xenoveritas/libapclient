/*! \file packets.h
 * \brief Data structures that the Archipelago client/server send each other.
 *
 * This contains the definitions for the following packets:
 *
 * | Packet Name       | Sent By | Notes                                 |
 * |-------------------|---------|---------------------------------------|
 * | Bounced           | Server  | Bounced message (see Bounce)          |
 * | Connected         | Server  | Indicates connection is complete      |
 * | ConnectionRefused | Server  | When a connection error happens       |
 * | DataPackage       | Server  | Data package                          |
 * | InvalidPacket     | Server  | Error message about a bad packet      |
 * | LocationInfo      | Server  | LocationScout response                |
 * | PrintJSON         | Server  | Text message                          |
 * | ReceivedItems     | Server  | Items from other worlds               |
 * | RoomInfo          | Server  | Basic room info                       |
 * | RoomUpdate        | Server  | Update to room info (eg, hint points) |
 * | Retrieved         | Server  | Server response for a Get             |
 * | SetReply          | Server  | Server response for a Set             |
 * | Bounce            | Client  |                                       |
 * | Connect           | Client  | Client connection request             |
 * | ConnectUpdate     | Client  |                                       |
 * | CreateHints       | Client  |                                       |
 * | Get               | Client  |                                       |
 * | GetDataPackage    | Client  | Only valid before sending Connect     |
 * | LocationChecks    | Client  |                                       |
 * | LocationScouts    | Client  |                                       |
 * | Say               | Client  |                                       |
 * | Set               | Client  |                                       |
 * | SetNotify         | Client  |                                       |
 * | StatusUpdate      | Client  |                                       |
 * | Sync              | Client  | Has no payload                        |
 * | UpdateHint        | Client  |                                       |
 */

#ifndef LIBAPCLIENT_PACKETS_H_
#define LIBAPCLIENT_PACKETS_H_

#include <list>
#include <string>
#include <unordered_map>
#include <optional>

#include <nlohmann/json.hpp>
using json = nlohmann::json;

namespace archipelago {

// General Archipelago typedefs.
// These typedefs exist primarily to just document WHAT a given ID is.
// TODO: Currently just int. May become int64_t?
// IDs are currently limited to 53 bits, negative IDs are reserved by Archipelago.

/// \brief A player ID, this is unique within a room
typedef int player_id_t;
/// \brief A team ID, this is unique within a room
typedef int team_id_t;
/// \brief A team slot ID, this is unique within a team
typedef int team_slot_id_t;
/// \brief An item ID, this is unique within a single world
typedef int item_id_t;
/// \brief A location ID, this is unique within a single world
typedef int location_id_t;

namespace packets {

// Packet name constants.

/// \brief `"cmd"` value for RoomInfo packets
const auto kPacketRoomInfo = "RoomInfo";
/// \brief `"cmd"` value for ConnectionRefused packets
const auto kPacketConnectionRefused = "ConnectionRefused";
/// \brief `"cmd"` value for Connected packets
const auto kPacketConnected = "Connected";
/// \brief `"cmd"` value for ReceivedItems packets
const auto kPacketReceivedItems = "ReceivedItems";
/// \brief `"cmd"` value for LocationInfo packets
const auto kPacketLocationInfo = "LocationInfo";
/// \brief `"cmd"` value for RoomUpdate packets
const auto kPacketRoomUpdate = "RoomUpdate";
/// \brief `"cmd"` value for PrintJSON packets
const auto kPacketPrintJSON = "PrintJSON";
/// \brief `"cmd"` value for DataPackage packets
const auto kPacketDataPackage = "DataPackage";
/// \brief `"cmd"` value for Bounced packets
const auto kPacketBounced = "Bounced";
/// \brief `"cmd"` value for InvalidPacket packets
const auto kPacketInvalidPacket = "InvalidPacket";
/// \brief `"cmd"` value for Retrieved packets
const auto kPacketRetrieved = "Retrieved";
/// \brief `"cmd"` value for SetReply packets
const auto kPacketSetReply = "SetReply";
/// \brief `"cmd"` value for Connect packets
const auto kPacketConnect = "Connect";
/// \brief `"cmd"` value for ConnectUpdate packets
const auto kPacketConnectUpdate = "ConnectUpdate";
/// \brief `"cmd"` value for Sync packets
const auto kPacketSync = "Sync";
/// \brief `"cmd"` value for LocationChecks packets
const auto kPacketLocationChecks = "LocationChecks";
/// \brief `"cmd"` value for LocationScouts packets
const auto kPacketLocationScouts = "LocationScouts";
/// \brief `"cmd"` value for CreateHints packets
const auto kPacketCreateHints = "CreateHints";
/// \brief `"cmd"` value for UpdateHint packets
const auto kPacketUpdateHint = "UpdateHint";
/// \brief `"cmd"` value for StatusUpdate packets
const auto kPacketStatusUpdate = "StatusUpdate";
/// \brief `"cmd"` value for Say packets
const auto kPacketSay = "Say";
/// \brief `"cmd"` value for GetDataPackage packets
const auto kPacketGetDataPackage = "GetDataPackage";
/// \brief `"cmd"` value for Bounce packets
const auto kPacketBounce = "Bounce";
/// \brief `"cmd"` value for Get packets
const auto kPacketGet = "Get";
/// \brief `"cmd"` value for Set packets
const auto kPacketSet = "Set";
/// \brief `"cmd"` value for SetNotify packets
const auto kPacketSetNotify = "SetNotify";

/*!
 * \brief Base Packet class.
 *
 * Provides the stubs necessary to send a packet to the Archipelago MultiWorld
 * server - namely the `cmd` field and a method to convert the packet to JSON.
 */
class Packet {
public:
    /*!
     * \brief Construct a packet.
     *
     * The `cmd` field is required - it identifies the packet type.
     *
     * \param pCmd the `cmd` field for this packet
     */
    Packet(const std::string&& pCmd) : cmd(pCmd) {}

    /*! \brief The name of this packet.
     *
     * All packets must have a cmd. This is used by the server to determine what
     * the packet is asking for and what arguments are necessary. This can be
     * changed at runtime if necessary, although doing so will alter the way the
     * packet works.
     */
    std::string cmd;

    /*!
     * \brief Base JSON converter.
     *
     * Calls convert_to_json(json &) to create the basic object, and then adds
     * the cmd field.
     *
     * \param j the JSON object to populate
     */
    void to_json(json& j) const {
        convert_to_json(j);
        j["cmd"] = cmd;
    }
protected:
    /*!
     * \brief Convert this packet to a JSON object with the fields the
     * Archipelago MultiWorld server expects.
     *
     * This is called with an empty JSON by to_json(json &). The object's `cmd`
     * field will be set after the object is populated - this allows the usage
     * of the JSON object constructor:
     *
     * ```cpp
     * j = json { { "field", "value" } };
     * ```
     *
     * \param j the JSON object to populate
     */
    virtual void convert_to_json(json& j) const = 0;
};

/// \brief An object representing software version.
struct NetworkVersion {
    NetworkVersion(const NetworkVersion& other) : major(other.major), minor(other.minor), build(other.build) {}
    NetworkVersion(int pMajor, int pMinor, int pBuild): major(pMajor), minor(pMinor), build(pBuild) {}
    NetworkVersion() : major(0), minor(0), build(0) {}
    int major;
    int minor;
    int build;
    bool operator==(const NetworkVersion& other) const;
    bool operator<(const NetworkVersion& other) const;
    /*! \brief Converts the version to a version string
     *
     * Converts to "{major}.{minor}.{build}", so:
     *
     * ```cpp
     * static_cast<string>(NetworkVersion(0, 6, 7))
     * ```
     *
     * Becomes the string `"0.6.7"`.
     *
     * \return the version string
     */
    operator std::string() const {
        return std::format("{}.{}.{}", major, minor, build);
    }
};

/*! \brief The possible client states that may be sent to the server in
 * StatusUpdate.
 */
enum class ClientStatus {
    /// \brief the default starting state
    unknown = 0,
    /*! \brief the client has connected, but the player isn't ready yet
     *
     * The state for our client will be set to this if it was
     * ClientStatus::unknown on our first connection.
     */
    connected = 5,
    /// \brief the client has indicated they are ready to start
    ready = 10,
    /// \brief the client is playing the game
    playing = 20,
    /// \brief the client has achieved their goal
    goal = 30
};

/*! \brief The possible command permission, for commands that may be restricted.
 */
enum Permission {
    /// \brief completely disables access
    disabled = 0b000,
    /// \brief allows manual use
    enabled = 0b001,
    /// \brief allows manual use after goal completion
    goal = 0b010,
    /// \brief forces use after goal completion, only works for release and collect
    automatic = 0b110,
    /// \brief forces use after goal completion, allows manual use any time
    autoEnabled = 0b111
};

/*!
 * \brief Flags that describe an item.
 *
 * Conceptually any or all of these flags may be set on an item.
 * `progression | useful` marks an especially useful item. Technically
 * `trap` may be set with `progression` and `useful` as well.
 */
enum class NetworkItemFlag {
    progression = 0b001,
    useful = 0b010,
    trap = 0b100
};

/*! \brief The basic metadata for an Archipelago item.
 */
struct NetworkItem {
    /// \brief the item id of the item.
    item_id_t item{ 0 };
    /// \brief the location id of the item inside the world.
    location_id_t location{ 0 };
    /*! \brief the player slot of the world the item is located in, except when
     * inside a LocationInfo Packet then it will be the slot of the player to
     * receive the item
     */
    player_id_t player{ 0 };
    int flags{ 0 };
    bool isProgression() { return flags & (int)NetworkItemFlag::progression; }
    bool isUseful() { return flags & (int)NetworkItemFlag::useful; }
    bool isTrap() { return flags & (int)NetworkItemFlag::trap; }
};

/// \brief The type of slot in a game.
enum SlotType {
    /// \brief only watching
    spectator = 0b00,
    /// \brief a player
    player = 0b01,
    /// \brief a group
    group = 0b10
};

/*! \brief Information about a slot.
 */
struct NetworkSlot {
    /// \brief the name for that slot
    std::string name{};
    /// \brief the game being played in that slot
    std::string game{};
    /// \brief the type of slot
    SlotType type{ SlotType::player };
    /// \brief group members(?)
    std::vector<player_id_t> group_members{};
};

/*! \brief Information about a player connected to the server.
 */
struct NetworkPlayer {
    team_id_t team;
    team_slot_id_t slot;
    std::string alias;
    std::string name;
};

//
// Server Packets (sent to the client)
//

/*! \brief A packet indicating that a Connect packet was refused.
 */
class ConnectionRefused : public Packet {
public:
    /*! \brief A list of error tags.
     *
     * Known errors are provided as class constants for this packet. It is
     * possible that in the future, new errors may be added.
     *
     * The server will only send back checked errors.
     */
    std::vector<std::string> errors;
    ConnectionRefused() : Packet(kPacketConnectionRefused), errors() {}
    ConnectionRefused(const ConnectionRefused&) = default;
    // The existance of constructors that take a list of strings breaks the
    // template magic that allows casting from a JSON object to this type.
    // Provide an explicit constructor to work for the default.
    ConnectionRefused(const json& jsonData);
    ConnectionRefused(const std::vector<std::string>&& errors) : Packet(kPacketConnectionRefused), errors(errors) {}
    ConnectionRefused(std::initializer_list<std::string> errors) : Packet(kPacketConnectionRefused), errors(errors) {}

    /*! \brief Searches the list to see if the given error is in it.
     *
     * \param error the error to check
     * \return true if the error is in the error list, false otherwise
     */
    bool has_error(const std::string& error) const;

    /*! \brief Searches the list to see if the given error is in it.
     *
     * \param error the error to check
     * \return true if the error is in the error list, false otherwise
     */
    bool has_error(const char* error) const { return has_error(std::string(error)); }

    virtual void convert_to_json(json& j) const override;

    // The following are static constants that describe error codes that exist within the Archipelago 0.6.7 server

    /*! \brief Indicates that the password given in the Connect packet does not
     * match the room's password.
     */
    static constexpr auto kInvalidPassword = "InvalidPassword";

    /*! \brief Indicates that the name given in the Connect packet does not
     * belong to any player in the room.
     */
    static constexpr auto kInvalidSlot = "InvalidSlot";

    /*! \brief Indicates that the game given in the Connect packet does not
     * match the game the player is playing.
     */
    static constexpr auto kInvalidGame = "InvalidGame";

    /*! \brief Indicates that the client version given in the Connect packet is
     * incompatible with the server.
     */
    static constexpr auto kIncompatibleVersion = "IncompatibleVersion";

    /*! \brief Indicates that the items_handling value given in the Connect
     * packet is incompatible with the server.
     */
    static constexpr auto kInvalidItemsHandling = "InvalidItemsHandling";
};

/*! \brief Slot info is documented as being a map of slot IDs to NetworkSlot
 * objects.
 *
 * Which is true, except JSON requires all keys in an object to be strings. This
 * class exists to handle converting to/from a map with string keys to a map
 * with `int` keys.
 */
class SlotInfo {
public:
    std::unordered_map<player_id_t, NetworkSlot> slot_info{};

    const NetworkSlot* getNetworkSlotByName(const std::string& name) const;
};

/*! \brief A packet sent by the server that describes the game being played.
 *
 * This is the first packet sent by the server.
 */
class RoomInfo : public Packet {
public:
    RoomInfo() : Packet(kPacketRoomInfo), version(), generator_version(), tags(), password(false), permissions(), hint_cost(0), location_check_points(0), games(), datapackage_checksums(), seed_name(), time(0.0) {}

    /// \brief the version of Archipelago which the server is running.
    NetworkVersion version;

    /// \brief the version of Archipelago which generated the multiworld.
    NetworkVersion generator_version;

    /// \brief the server tags
    std::vector<std::string> tags;

    /// \brief whether the room requires a password
    bool password;

    /// \brief a map of commands to permissions to use them
    std::unordered_map<std::string, Permission> permissions;

    /*! \brief The percentage of total locations that need to be checked to
     * receive a hint from the server.
     */
    int hint_cost;

    /*! \brief The amount of hint points you receive per item / location check
     * completed.
     */
    int location_check_points;

    /// \brief List of games present in this multiworld.
    std::vector<std::string> games;

    /*! \brief Checksum hash of the individual games' data packages the server
     * will send.
     *
     * Used by newer clients to decide which games' caches are outdated. See
     * Data Package Contents for more information.
     */
    std::unordered_map<std::string, std::string> datapackage_checksums;

    /*! \brief The seed name, which is unqiue per seed/randomization method.
     *
     * Games should check to make sure this matches what they expect.
     */
    std::string seed_name;

    /*! \brief Unix time stamp from the server, indicating the time it thinks
     * it is.
     */
    double time;
    virtual void convert_to_json(json& j) const override;
};

/*! \brief Sent to clients when the connection handshake is successfully
 * completed.
 */
class Connected : public Packet {
public:
    /// \brief Your team number. See NetworkPlayer for more info on team number.
    team_id_t team{ 0 };

    /*! \brief Your slot number on your team. See NetworkPlayer for more info on
     * the slot number.
     */
    team_slot_id_t slot{ 0 };

    /*! \brief List denoting other players in the multiworld, whether connected
     * or not.
     */
    std::vector<NetworkPlayer> players{};

    /*! \brief Contains ids of remaining locations that need to be checked.
     * Useful for trackers, among other things.
     */
    std::vector<location_id_t> missing_locations{};

    /*! \brief Contains ids of all locations that have been checked.
     */
    std::vector<location_id_t> checked_locations{};

    /*! \brief Contains a json object for slot related data, differs per game.
     *
     * Empty if not required.
     * Not present if slot_data in Connect is false.
     */
    json slot_data;

    /*! \brief maps each slot to a NetworkSlot information.
     */
    SlotInfo slot_info;

    /*! \brief Number of hint points that the current player has.
     */
    // TODO: Could be unsigned?
    int hint_points{ 0 };

    Connected() : Packet(kPacketConnected), slot_data(), slot_info() {}
    virtual void convert_to_json(json& j) const override;

    /*! \brief Look up a player's information based on their name.
     *
     * As the player data is a std::vector, this takes up to O(n) time.
     */
    const NetworkPlayer* getPlayer(const std::string& name) const;
};

/*! \brief A packet indicating changes in room info.
 *
 * This is somewhat poorly documented, the fields present here are based on the
 * Archipelago source code for 0.6.7.
 */
class RoomUpdate : public Packet {
public:
    RoomUpdate() : Packet(kPacketRoomUpdate) {}
    std::optional<int> hint_points;
    std::optional<int> location_check_points;
    std::optional<std::vector<NetworkPlayer>> players;
    std::optional<std::vector<location_id_t>> checked_locations;
    std::optional<std::unordered_map<std::string, Permission>> permissions;
    virtual void convert_to_json(json& j) const override;
};

/*! \brief A packet indicating items have been sent to this world.
 */
class ReceivedItems : public Packet {
public:
    /*! \brief The next empty slot in the list of items for the receiving
     * client.
     */
    item_id_t index;
    /*! \brief The items which the client is receiving.
     */
    std::vector<NetworkItem> items;
    /*! \brief Create an empty ReceivedItems packet.
     */
    ReceivedItems() : Packet(kPacketReceivedItems), index(0), items() {}
    virtual void convert_to_json(json& j) const override;
};

/*! \brief A response to a LocationScouts packet. Provides information on the
 * locations requested.
 */
class LocationInfo : public Packet {
public:
    /// \brief Contains list of item(s) in the location(s) scouted.
    std::vector<NetworkItem> locations;
    LocationInfo() : Packet(kPacketLocationInfo) {}
    virtual void convert_to_json(json & j) const override;
};

/*! \brief the status of a hint
 */
enum class HintStatus {
    /// \brief The receiving player has not specified any status
    HINT_UNSPECIFIED = 0,
    /// \brief The receiving player has specified that the item is unneeded
    HINT_NO_PRIORITY = 10,
    /// \brief The receiving player has specified that the item is detrimental
    HINT_AVOID = 20,
    /// \brief The receiving player has specified that the item is needed
    HINT_PRIORITY = 30,
    /// \brief The location has been collected. Status cannot be changed once found.
    HINT_FOUND = 40
};


/*! \brief The type of the PrintJSON message.
 *
 * Type names are taken from Archipelago, which calls the packet PrintJSON and
 * the type PrintJsonType, which is why this isn't PrintJSONType.
 *
 * Conversion functions exist to translate this to and from JSON.
 */
enum class PrintJsonType {
    /// \brief No type was given
    none,
    /// \brief The type could not be parsed.
    unknown,
    /// \brief A player received an item.
    ItemSend,
    /// \brief A player used the !getitem command.
    ItemCheat,
    /// \brief A player hinted.
    Hint,
    /// \brief A player connected.
    Join,
    /// \brief A player disconnected.
    Part,
    /// \brief A player sent a chat message.
    Chat,
    /// \brief The server broadcasted a message.
    ServerChat,
    /// \brief The client has triggered a tutorial message, such as when first connecting.
    Tutorial,
    /// \brief A player changed their tags.
    TagsChanged,
    /// \brief Someone (usually the client) entered a !command.
    CommandResult,
    /// \brief The client entered an !admin command.
    AdminCommandResult,
    /// \brief A player reached their goal.
    Goal,
    /// \brief A player released the remaining items in their world.
    Release,
    /// \brief A player collected the remaining items for their world.
    Collect,
    /// \brief The current server countdown has progressed.
    Countdown
};

/*! \brief Part of a PrintJSON packet.
 *
 * This can be used to indicate special information which may be rendered
 * differently depending on client.
 */
enum class JSONMessagePartType {
    /// \brief Regular text content.
    text,
    /// \brief player ID of someone on your team, should be resolved to Player Name
    player_id,
    /// \brief Player Name, could be a player within a multiplayer game or from another team, not ID resolvable
    player_name,
    /// \brief Item ID, should be resolved to Item Name
    item_id,
    /// \brief Item Name, not currently used over network, but supported by reference Clients.
    item_name,
    /// \brief Location ID, should be resolved to Location Name
    location_id,
    /// \brief Location Name, not currently used over network, but supported by reference Clients.
    location_name,
    /// \brief Entrance Name. No ID mapping exists.
    entrance_name,
    /// \brief The HintStatus of the hint. Both text and hint_status are given.
    hint_status,
    /// \brief Regular text that should be colored. Only type that will contain color data.
    color
};

/// \brief Pre-defined colors
enum class JSONMessagePartColor {
    bold,
    underline,
    black,
    red,
    green,
    yellow,
    blue,
    magenta,
    cyan,
    white,
    black_bg,
    red_bg,
    green_bg,
    yellow_bg,
    blue_bg,
    magenta_bg,
    cyan_bg,
    white_bg
};

/// \brief Parts of a message in a PrintJSON packet.
class JSONMessagePart {
public:
    /// @brief Raw type string, if it was given
    std::optional<std::string> type;
    std::optional<std::string> text;
    std::optional<std::string> color;
    std::optional<int> flags;
    std::optional<player_id_t> player;
    std::optional<HintStatus> hint_status;
};

/*! \brief Sent to clients purely to display a message to the player.
 *
 * The data field is the only important field - the other information is
 * metadata that can be used to determine how the text should be displayed and
 * provide additional information to the user.
 */
class PrintJSON : public Packet {
public:
    /// \brief Textual content of this message
    std::vector<JSONMessagePart> data{};
    /// \brief the type of message
    PrintJsonType type{ PrintJsonType::none };
    /// \brief Destination player's ID (only valid on ItemSend, ItemCheat, Hint)
    std::optional<player_id_t> receiving{};
    /// \brief Source player's ID, location ID, item ID and item flags (only valid on ItemSend, ItemCheat, Hint)
    std::optional<NetworkItem> item{};
    /// \brief Whether the location hinted for was checked (only valid on Hint)
    std::optional<bool> found{};
    /// \brief Team of the triggering player (valid on Join, Part, Chat, TagsChanged, Goal, Release, Collect, ItemCheat)
    std::optional<team_id_t> team{};
    /// \brief Slot of the triggering player (valid on Join, Part, Chat, TagsChanged, Goal, Release, Collect)
    std::optional<team_slot_id_t> slot{};
    /// \brief Original chat message without sender prefix (only valid on Chat, ServerChat)
    std::optional<std::string> message{};
    /// \brief Tags of the triggering player (only valid on Join, TagsChanged)
    std::optional<std::vector<std::string>> tags{};
    /// \brief Amount of seconds remaining on the countdown
    std::optional<int> countdown{};
    PrintJSON() : Packet(kPacketPrintJSON) {}
    virtual void convert_to_json(json& j) const override;
};

/*! \brief Sent to clients after a client requested this message be sent to
 * them, more info in the Bounce package.
 */
class Bounced : public Packet {
public:
    /// \brief Game names this message is targeting
    std::optional<std::vector<std::string>> games;
    /// \brief Player slot IDs that this message is targeting
    std::optional<std::vector<team_slot_id_t>> slots;
    /// \brief Client Tags this message is targeting
    std::optional<std::vector<std::string>> tags;
    /// \brief The actual data sent
    std::optional<json> data;
    Bounced() : Packet(kPacketBounced) {}
    virtual void convert_to_json(json& j) const override;
};

/*! \brief Receives a data package from the server.
 *
 * The data package is the package generated by World object's
 * `get_data_package_data()` class method. By default, it contains:
 *
 * | Field name            | Type               | Description                        |
 * |-----------------------|--------------------|------------------------------------|
 * | `item_name_to_id`     | `map<string, int>` | Map of item names to their IDs     |
 * | `location_name_to_id` | `map<string, int>` | Map of location names to their IDs |
 *
 * However, it can contain arbitrary data, so this class doesn't force a
 * specific decoding and instead stores the raw JSON data.
 */
class DataPackage : public Packet {
public:
    DataPackage() : Packet(kPacketDataPackage), games() {}
    /*! \brief The data packages for each game.
     *
     * The JSON generated for this places the games within a single JSON field
     * called "data". Rather than represent that in C++, this handles that
     * detail during conversion to/from JSON and otherwise just stores the
     * games map.
     */
    std::unordered_map<std::string, json> games;
    virtual void convert_to_json(json& j) const override;
};

/// @brief Sent to clients if the server caught a problem with a packet. This only occurs for errors that are explicitly checked for.
class InvalidPacket : public Packet {
public:
    /// \brief The PacketProblemType that was detected in the packet.
    std::string type{};
    /// \brief The cmd argument of the faulty packet, will be unset if the cmd failed to be parsed.
    std::optional<std::string> original_cmd{};
    /// \brief A descriptive message of the problem at hand.
    std::string text{};
    InvalidPacket() : Packet(kPacketInvalidPacket) {}
    InvalidPacket(const std::string& type, const std::string& text) : Packet(kPacketInvalidPacket), type(type), text(text) {}
    virtual void convert_to_json(json& j) const override;
};

/// @brief indicates the type of problem that was detected in the faulty packet, the known problem types are below but others may be added in the future.
namespace PacketProblemType {
    // The command was invalid
    const auto cmd = "cmd";
    // One of the arguments was invalid
    const auto arguments = "arguments";
};

/// \brief Response from a Get packet with the requested items.
class Retrieved : public Packet {
public:
    std::unordered_map<std::string, json> keys;
    Retrieved() : Packet(kPacketRetrieved) {}
    virtual void convert_to_json(json& j) const override;
};

/*! \brief Sent to clients in response to a Set package if `want_reply` was set
 * to true, or if the client has registered to receive updates for a certain
 * key using SetNotify.
 *
 * SetReply packets are sent even if a Set package did not alter the value for the key.
 */
class SetReply: public Packet {
public:
    /// \brief The key that was updated.
    std::string key;
    /// \brief The new value for the key.
    json value;
    /*! \brief The value the key had before it was updated.
     *
     * Not present (set to `std::nullopt`) on `"_read"` prefixed special keys.
     */
    std::optional<json> original_value{ std::nullopt };
    /// \brief The slot that originally sent the Set package causing this change.
    team_slot_id_t slot{ 0 };
    SetReply() : Packet(kPacketSetReply), value{ nullptr } {}
    virtual void convert_to_json(json& j) const override;
};

//
// Client Packets (sent to the server)
//

/// \brief Valid flags for ItemsHandling.
enum class ItemsHandling {
    /// \brief Never receive information about items.
    none = 0b000,
    /// \brief Only receive notifications of world items found by other players.
    otherWorlds = 0b001,
    /// \brief Receive notification of items others find and found in the local world.
    otherAndOwn = 0b011,
    /// \brief Receive notification of items others find and placed in the starting inventory.
    otherAndStarting = 0b101,
    /// \brief Receive notification of items others find, found in this world, and placed in the starting inventory.
    all = 0b111
};

/*! \brief Sent by the client to initiate a connection to an Archipelago game
 * session.
 */
class Connect : public Packet {
public:
    /// \brief the password for the game, if one is required
    std::optional<std::string> password{};
    /// \brief the name of the game the client is playing (should match the AP World)
    /// Optional only when one of the tags is Tracker, HintGame, or TextOnly.
    std::optional<std::string> game{};
    /// \brief the player name for this client
    std::string name{ "" };
    /// \brief Unique identifier for player. See getPlayerUUID() for details.
    std::string uuid{ "" };
    /*! \brief The Archipelago version this client supports.
     *
     * Defaults to 0.6.7, the version this library was tested against. Earlier
     * versions may also work.
     *
     * The server may reject the connection if it knows it can't support this
     * client.
     */
    NetworkVersion version{ 0, 6, 7 };
    /*! \brief Flags indicating the items the client wants sent via
     * ReceivedItem packets.
     */
    ItemsHandling items_handling{ ItemsHandling::none };

    /*! \brief Client tags.
     *
     * See archipelago::tags for a list of known tags.
     */
    std::vector<std::string> tags;
    /// \brief If true, the Connect answer will contain slot_data
    bool slot_data;
    virtual void convert_to_json(json& json) const override;
    Connect() : Packet(kPacketConnect), password(), game(), name(), uuid(), items_handling(ItemsHandling::none), tags(), slot_data(false) {}
    /// Create a Connect packet with the given game name. This will default items_handling to
    /// \param game the game name to use in the packet
    Connect(const std::string& game) : Packet(kPacketConnect), password(), game(game), name(), uuid(), items_handling(ItemsHandling::otherWorlds), tags(), slot_data(false) {}
};

/*! \brief Update arguments from the Connect package.
 *
 * Currently only updating tags and items_handling is supported by Archipelago.
 */
class ConnectUpdate : public Packet {
public:
    /// \brief Flags configuring which items should be sent by the server.
    std::optional<ItemsHandling> items_handling{};
    /// \brief Denotes special features or capabilities that the sender is capable of.
    std::optional<std::vector<std::string>> tags{};
    ConnectUpdate() : Packet(kPacketConnectUpdate) {}
    virtual void convert_to_json(json& json) const override;
};

class LocationChecks : public Packet {
public:
    // List of location IDs that have been checked by the client. May include checks that have already been reported to the server.
    std::vector<location_id_t> locations;

    LocationChecks() : Packet(kPacketLocationChecks), locations() {}
    LocationChecks(const std::vector<location_id_t>& locations) : Packet(kPacketLocationChecks), locations(locations) {}
    virtual void convert_to_json(json& json) const override;
};

/*! \brief Sent to the server to retrieve the items that are on a specified list
 * of locations.
 *
 * The server will respond with a LocationInfo packet containing the items
 * located in the scouted locations. Fully remote clients without a patch file
 * may use this to "place" items onto their in-game locations, most commonly to
 * display their names or item classifications before / upon pickup.
 *
 * LocationScouts can also be used to inform the server of locations the client
 * has seen, but not checked. This creates a hint as if the player had run
 * !hint_location on a location, but without deducting hint points.
 *
 * This is useful in cases where an item appears in the game world, such as
 * 'ledge items' in A Link to the Past. To do this, set the create_as_hint
 * parameter to a non-zero value.
 */
class LocationScouts : public Packet {
public:
    /// \brief The ids of the locations seen by the client.
    std::vector<location_id_t> locations{};
    /*! \brief If non-zero, the scouted locations get created and broadcasted as
     * a player-visible hint.
     */
    int create_as_hint{ 0 };
    LocationScouts() : Packet(kPacketLocationScouts) {}
    virtual void convert_to_json(json& json) const override;
};

/*! \brief Sent to the server to create hints for a specified list of locations.
 *
 * Hints that already exist will be silently skipped and their status will not
 * be updated. When creating hints for another slot's locations, the packet will
 * fail if any of those locations don't contain items for the requesting slot.
 * When creating hints for your own slot's locations, non-existing locations
 * will silently be skipped.
 */
class CreateHints : public Packet {
public:
    std::vector<location_id_t> locations;
    player_id_t player{ 0 };
    /// If included, sets the status of the hint to this status.
    /// Defaults to HINT_UNSPECIFIED. Cannot set HINT_FOUND.
    std::optional<HintStatus> status{ std::nullopt };
    CreateHints() : Packet(kPacketCreateHints) {}
    virtual void convert_to_json(json& json) const override;
};

/*! \brief Sent to the server to update the status of a Hint.
 *
 * The client must be the receiving `player` of the Hint, or the update fails.
 */
class UpdateHint : public Packet {
public:
    /// \brief the ID of the player being updated
    player_id_t player;
    /// \brief the location ID of the hint being updated
    location_id_t location;
    /// \brief the new hint status
    std::optional<HintStatus> status;

    UpdateHint() : Packet(kPacketUpdateHint), player(0), location(0), status(std::nullopt) {}
    virtual void convert_to_json(json& json) const override;
};

/*! \brief Sent to the server to update on the sender's status.
 *
 * Examples include readiness or goal completion. (Example: defeated Ganon in A
 * Link to the Past)
 */
class StatusUpdate : public Packet {
public:
    StatusUpdate() : Packet(kPacketStatusUpdate), status(ClientStatus::unknown) {}
    StatusUpdate(const StatusUpdate&) = default;
    StatusUpdate(ClientStatus pStatus) : Packet(kPacketStatusUpdate), status(pStatus) {}
    ClientStatus status;
    virtual void convert_to_json(json& json) const override;
};

/*! \brief A Sync packet. This packet has no payload and therefore contains no
 * members. This class exists solely for completeness.
 */
class Sync : public Packet {
public:
    Sync() : Packet(kPacketSync) {}
    void convert_to_json(json& j) const override;
};

/*! \brief Send text to other players.
 */
class Say : public Packet {
public:
    Say() : Packet(kPacketSay), text() {}
    Say(const std::string& text) : Packet(kPacketSay), text(text) {}
    Say(const char* text) : Packet(kPacketSay), text(text) {}
    /// \brief The text to say
    std::string text;
    virtual void convert_to_json(json& json) const override;
};

/*! \brief Requests the data package from the server.
 *
 * This packet will only function *after* a RoomInfo packet has been received
 * and *before* a Connect packet is sent.
 */
class GetDataPackage : public Packet {
public:
    GetDataPackage() : Packet(kPacketGetDataPackage), games() {}
    GetDataPackage(const std::vector<std::string>& pGames) : Packet(kPacketGetDataPackage), games(pGames) {}
    /*! \brief A list of games to retrieve.
     *
     * If given, the server will only return games belonging to this set.
     */
    std::optional<std::vector<std::string>> games;
    virtual void convert_to_json(json& json) const override;
};

/*! \brief Send a message to the server that will be relayed (bounced) to other
 * clients.
 */
class Bounce: public Packet {
public:
    /// \brief Game names that should receive this message
    std::optional<std::vector<std::string>> games;
    /// \brief Player IDs that should receive this message
    std::optional<std::vector<player_id_t>> slots;
    /// \brief Client tags that should receive this message
    std::optional<std::vector<std::string>> tags;
    /// \brief Any data you want to send
    std::optional<json> data;
    Bounce() : Packet(kPacketBounce) {}
    virtual void convert_to_json(json& j) const override;
};

/*! \brief A Bounce packet that indicates that a player has died.
 */
class DeathLink {
    /*! \brief Unix Time Stamp of time of death.
     *
     * See Client::get_remote_time_now() for a method of generating this time
     * stamp.
     */
    double time;

    /*! \brief Text to explain the cause of death.
     *
     * When provided, or checked, if the string is non-empty, it should contain
     * the player name, ex. "Berserker was run over by a train."
     */
    std::optional<std::string> cause;

    /*! \brief Name of the player who first died.
     *
     * Can be a slot name, but can also be a name from within a multiplayer game.
     */
    std::string source;
};

/*! \brief Used to request a single or multiple values from the server's data
 * storage.
 *
 * See the Set package for how to write values to the data storage. A Get packet
 * will be answered with a Retrieved packet.
 *
 * Archipelago allows extra JSON fields to be included in the Get packet that will
 * then be returned in the associated Retrieved packet.
 */
class Get : public Packet {
public:
    /// \brief Keys to retrieve the values for.
    std::vector<std::string> keys;
    Get() : Packet(kPacketGet), keys() {}
    Get(const std::string& key) : Packet(kPacketGet), keys({ key }) {}
    Get(const std::vector<std::string>& keys) : Packet(kPacketGet), keys(keys) {}
    virtual void convert_to_json(json& j) const override;
};

/*! \brief operation to perform on a value when setting it via a Set packet.
 */
enum class DataStorageOperationType {
    /// \brief Sets the current value of the key to value.
    opReplace,
    /// \brief If the key has no value yet, sets the current value of the key to default of the Set's package (value is ignored).
    opDefault,
    /// \brief Adds value to the current value of the key, if both the current value and value are arrays then value will be appended to the current value.
    opAdd,
    /// \brief Multiplies the current value of the key by value.
    opMul,
    /// \brief Multiplies the current value of the key to the power of value.
    opPow,
    /// \brief Sets the current value of the key to the remainder after division by value.
    opMod,
    /// \brief Floors the current value(value is ignored).
    opFloor,
    /// \brief Ceils the current value(value is ignored).
    opCeil,
    /// \brief Sets the current value of the key to value if value is bigger.
    opMax,
    /// \brief Sets the current value of the key to value if value is lower.
    opMin,
    /// \brief Applies a bitwise AND to the current value of the key with value.
    opAnd,
    /// \brief Applies a bitwise OR to the current value of the key with value.
    opOr,
    /// \brief Applies a bitwise Exclusive OR to the current value of the key with value.
    opXor,
    /// \brief Applies a bitwise left - shift to the current value of the key by value.
    opLeftShift,
    /// \brief Applies a bitwise right - shift to the current value of the key by value.
    opRightShift,
    /// \brief List only : removes the first instance of value found in the list.
    opRemove,
    /// \brief List or Dict : for lists it will remove the index of the value given. for dicts it removes the element with the specified key of value.
    opPop,
    /// \brief List or Dict : Adds the elements of value to the container if they weren't already present. In the case of a Dict, already present keys will have their corresponding values updated.
    opUpdate
};

/*! \brief describes how to set data
 */
struct DataStorageOperation {
    /// \brief the operation to perform
    DataStorageOperationType operation{ DataStorageOperationType::opDefault };
    /// \brief the new JSON value
    json value{};
};

/*! \brief Used to write data to the server's data storage.
 *
 * That data can then be shared across worlds or just saved for later.
 *
 * Values for keys in the data storage can be retrieved with a Get packet, or
 * monitored with a SetNotify packet. Keys that start with `_read_` cannot be
 * set.
 */
class Set : public Packet {
public:
    /// \brief The key to manipulate. Can never start with `_read`.
    std::string key{};
    /*! \brief The default value to use in case the key has no value on the
     * server.
     *
     * The actual field in the JSON object is `"default"` but as `default` is
     * a keyword, the field name needed to be something else.
     */
    json default_value{ nullptr };
    /// \brief If true, the server will send a SetReply response back to the client.
    bool want_reply{ false };
    /// \brief Operations to apply to the value, multiple operations can be present and they will be executed in order of appearance.
    std::vector<DataStorageOperation> operations{};
    Set() : Packet(kPacketSet) {}
    Set(const std::string& key) : Packet(kPacketSet), key(key) {}
    Set(const std::string& key,
        const json& default_value,
        bool want_reply = false,
        const std::vector<DataStorageOperation> operations = std::vector<DataStorageOperation>()
    ) : Packet(kPacketSet),
        key(key),
        default_value(default_value),
        want_reply(want_reply),
        operations(operations) {}
    virtual void convert_to_json(json& j) const override;
};

/*! \brief Requests that the server notify a client whenever a value changes.
 */
class SetNotify : public Packet {
public:
    /// \brief Keys to receive all SetReply packages for.
    std::vector<std::string> keys;
    SetNotify() : Packet(kPacketSetNotify) {}
    virtual void convert_to_json(json& j) const override;
};

//
// CONVERSION FUNCTIONS
//
// Functions that allow the JSON library to convert to/from JSON objects via typecasts
// (Alphabetized for the sake of sanity.)
//
// There's no good reason to list these in Doxygen so ... don't.
/// \cond
void from_json(const json& j, Bounce& bounce);
void to_json(json& j, const Bounce& bounce);
void from_json(const json& j, Bounced& bounced);
void to_json(json& j, const Bounced& bounced);
void from_json(const json& j, Connect& connect);
void to_json(json& j, const Connect& connect);
void from_json(const json& j, Connected& connected);
void to_json(json& j, const Connected& connected);
void from_json(const json& j, ConnectionRefused& connectionRefused);
void to_json(json& j, const ConnectionRefused& connectionRefused);
void from_json(const json& j, ConnectUpdate& connectUpdate);
void to_json(json& j, const ConnectUpdate& connectUpdate);
void from_json(const json& j, CreateHints& createHints);
void to_json(json& j, const CreateHints& createHints);
void from_json(const json& j, DataPackage& dataPackage);
void to_json(json& j, const DataPackage& dataPackage);
void from_json(const json& j, DataStorageOperation& dataStorageOperation);
void to_json(json& j, const DataStorageOperation& dataStorageOperation);
void from_json(const json& j, DataStorageOperationType& dataStorageOperationType);
void to_json(json& j, const DataStorageOperationType& dataStorageOperationType);
void from_json(const json& j, Get& get);
void to_json(json& j, const Get& get);
void from_json(const json& j, GetDataPackage& getDataPackage);
void to_json(json& j, const GetDataPackage& getDataPackage);
void from_json(const json& j, InvalidPacket& invalidPacket);
void to_json(json& j, const InvalidPacket& invalidPacket);
void from_json(const json& j, JSONMessagePart& jsonMessagePart);
void to_json(json& j, const JSONMessagePart& jSONMessagePart);
void from_json(const json& j, LocationChecks& locationChecks);
void to_json(json& j, const LocationChecks& locationChecks);
void from_json(const json& j, LocationInfo& locationInfo);
void to_json(json& j, const LocationInfo& locationInfo);
void from_json(const json& j, LocationScouts& locationScouts);
void to_json(json& j, const LocationScouts& locationScouts);
void from_json(const json& j, NetworkItem& networkItem);
void to_json(json& j, const NetworkItem& networkItem);
void from_json(const json& j, NetworkPlayer& networkPlayer);
void to_json(json& j, const NetworkPlayer& networkPlayer);
void from_json(const json& j, NetworkSlot& networkSlot);
void to_json(json& j, const NetworkSlot& networkSlot);
void from_json(const json& j, NetworkVersion& networkVersion);
void to_json(json& j, const NetworkVersion& networkVersion);
void from_json(const json& j, PrintJSON& printJSON);
void to_json(json& j, const PrintJSON& printJSON);
void from_json(const json& j, PrintJsonType& printJsonType);
void to_json(json& j, const PrintJsonType& printJsonType);
void from_json(const json& j, ReceivedItems& receivedItems);
void to_json(json& j, const ReceivedItems& receivedItems);
void from_json(const json& j, Retrieved& retrieved);
void to_json(json& j, const Retrieved& retrieved);
void from_json(const json& j, RoomInfo& roomInfo);
void to_json(json& j, const RoomInfo& roomInfo);
void from_json(const json& j, RoomUpdate& roomUpdate);
void to_json(json& j, const RoomUpdate& roomUpdate);
void from_json(const json& j, Say& say);
void to_json(json& j, const Say& say);
void from_json(const json& j, Set& set);
void to_json(json& j, const Set& set);
void from_json(const json& j, SetNotify& setNotify);
void to_json(json& j, const SetNotify& setNotify);
void from_json(const json& j, SetReply& setReply);
void to_json(json& j, const SetReply& setReply);
void from_json(const json& j, SlotInfo& slotInfo);
void to_json(json& j, const SlotInfo& slotInfo);
void from_json(const json& j, StatusUpdate& statusUpdate);
void to_json(json& j, const StatusUpdate& statusUpdate);
void from_json(const json& j, Sync& sync);
void to_json(json& j, const Sync& sync);
void from_json(const json& j, UpdateHint& updateHint);
void to_json(json& j, const UpdateHint& updateHint);
/// \endcond


}; // namespace packets

}; // namespace archipelago

#endif // LIBAPCLIENT_PACKETS_H_