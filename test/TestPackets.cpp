/*
 * Test file for ensuring packet encode/decode works as expected.
 */

#include "libapclient/packets.h"
#include "libapclient/logger.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

TEST(Packets, parseBounce) {
    // Check empty bounce
    json j = json::parse("{\"cmd\":\"Bounce\"}");
    auto packet = static_cast<archipelago::packets::Bounce>(j);
    EXPECT_FALSE(packet.games.has_value());
    EXPECT_FALSE(packet.slots.has_value());
    EXPECT_FALSE(packet.tags.has_value());
    EXPECT_FALSE(packet.data.has_value());
}

TEST(Packets, encodeBounce) {
    archipelago::packets::Bounce b;
    b.games = std::vector<std::string>({ "super test", "super test ultra deluxe edition" });
    b.slots = std::vector<archipelago::player_id_t>({ 42, 67 });
    b.tags = std::vector<std::string>({ "example" });
    std::string jsonStr = static_cast<json>(b).dump();
    EXPECT_EQ(jsonStr, "{\"cmd\":\"Bounce\",\"games\":[\"super test\",\"super test ultra deluxe edition\"],\"slots\":[42,67],\"tags\":[\"example\"]}");
}

TEST(Packets, parseBounced) {
    json j = json::parse("{\"cmd\":\"Bounced\"}");
    auto packet = static_cast<archipelago::packets::Bounced>(j);
    EXPECT_FALSE(packet.games.has_value());
    EXPECT_FALSE(packet.slots.has_value());
    EXPECT_FALSE(packet.tags.has_value());
    EXPECT_FALSE(packet.data.has_value());
}

TEST(Packets, encodeBounced) {
    archipelago::packets::Bounced b;
    std::string jsonStr = static_cast<json>(b).dump();
    EXPECT_EQ(jsonStr, "{\"cmd\":\"Bounced\"}");
}

TEST(Packets, parseConnect) {
    json j = json::parse("{\"cmd\":\"Connect\",\"game\":\"Game Name\",\"name\":\"Player Name\",\"password\":null,\"items_handling\":7,\"uuid\":\"a real UUID\",\"version\":{\"class\":\"Version\",\"major\":0,\"minor\":6,\"build\":7},\"slot_data\":false,\"tags\":[]}");
    auto packet = static_cast<archipelago::packets::Connect>(j);
    EXPECT_EQ(packet.game, "Game Name");
    EXPECT_EQ(packet.name, "Player Name");
    EXPECT_EQ(packet.uuid, "a real UUID");
    EXPECT_EQ(packet.version, archipelago::packets::NetworkVersion(0, 6, 7));
    EXPECT_FALSE(packet.password.has_value());
    EXPECT_EQ(packet.slot_data, false);
    EXPECT_TRUE(packet.tags.empty());
    EXPECT_EQ(packet.items_handling, archipelago::packets::ItemsHandling::all);
}

TEST(Packets, encodeConnect) {
    archipelago::packets::Connect c;
    c.name = "Player";
    c.uuid = "1234-5678";
    c.game = "A Game";
    std::string jsonStr = static_cast<json>(c).dump();
    EXPECT_EQ(jsonStr, "{\"cmd\":\"Connect\",\"game\":\"A Game\",\"items_handling\":0,\"name\":\"Player\",\"password\":null,\"slot_data\":false,\"tags\":[],\"uuid\":\"1234-5678\",\"version\":{\"build\":7,\"class\":\"Version\",\"major\":0,\"minor\":6}}");
}

TEST(Packets, parseConnected) {
    json j = json::parse("{\"cmd\":\"Connected\",\"team\":1,\"slot\":2,\"players\":[],\"missing_locations\":[2,4,6],\"checked_locations\":[1,3,5],\"slot_info\":{},\"hint_points\":4}");
    auto packet = static_cast<archipelago::packets::Connected>(j);
    EXPECT_EQ(packet.team, 1);
    EXPECT_EQ(packet.slot, 2);
    EXPECT_THAT(packet.missing_locations, testing::ElementsAreArray({ 2, 4, 6 }));
    EXPECT_THAT(packet.checked_locations, testing::ElementsAreArray({ 1, 3, 5 }));
    EXPECT_EQ(packet.hint_points, 4);
    EXPECT_THAT(packet.players, testing::IsEmpty());
    // Ignore slot_info
}

TEST(Packets, encodeConnected) {
    archipelago::packets::Connected c;
    c.team = 2;
    c.slot = 1;
    c.missing_locations = { 1, 2, 3 };
    c.checked_locations = { 4, 5, 6 };
    std::string jsonStr = static_cast<json>(c).dump();
    EXPECT_EQ(jsonStr, "{\"checked_locations\":[4,5,6],\"cmd\":\"Connected\",\"hint_points\":0,\"missing_locations\":[1,2,3],\"players\":[],\"slot\":1,\"slot_data\":null,\"slot_info\":{},\"team\":2}");
}

TEST(Packets, parseConnectionRefused) {
    json j = json::parse("{\"cmd\":\"ConnectionRefused\",\"errors\":[\"InvalidGame\",\"IncompatibleVersion\"]}");
    auto packet = static_cast<archipelago::packets::ConnectionRefused>(j);
    EXPECT_THAT(packet.errors, testing::ElementsAre(archipelago::packets::ConnectionRefused::kInvalidGame, archipelago::packets::ConnectionRefused::kIncompatibleVersion));
}

TEST(Packets, encodeConnectionRefused) {
    archipelago::packets::ConnectionRefused c({ archipelago::packets::ConnectionRefused::kInvalidItemsHandling });
    std::string jsonStr = static_cast<json>(c).dump();
    EXPECT_EQ(jsonStr, "{\"cmd\":\"ConnectionRefused\",\"errors\":[\"InvalidItemsHandling\"]}");
}

TEST(Packets, parseConnectUpdate) {
    json j = json::parse("{\"cmd\":\"ConnectUpdate\"}");
    auto packet = static_cast<archipelago::packets::ConnectUpdate>(j);
    // Blank is valid ... probably.
    EXPECT_THAT(packet.items_handling, testing::Eq(std::nullopt));
    EXPECT_THAT(packet.tags, testing::Eq(std::nullopt));
    j = json::parse("{\"cmd\":\"ConnectionUpdate\",\"items_handling\":3,\"tags\":[\"TextOnly\"]}");
    packet = static_cast<archipelago::packets::ConnectUpdate>(j);
    EXPECT_THAT(packet.items_handling, testing::Optional(archipelago::packets::ItemsHandling::otherAndOwn));
    EXPECT_THAT(packet.tags, testing::Optional(std::vector<std::string>({ "TextOnly" })));
}

TEST(Packets, encodeConnectUpdate) {
    archipelago::packets::ConnectUpdate c;
    std::string jsonStr = static_cast<json>(c).dump();
    // Guess, what, a blank packet is valid. Probably.
    EXPECT_EQ(jsonStr, "{\"cmd\":\"ConnectUpdate\"}");
    c.items_handling = archipelago::packets::ItemsHandling::otherWorlds;
    jsonStr = static_cast<json>(c).dump();
    EXPECT_EQ(jsonStr, "{\"cmd\":\"ConnectUpdate\",\"items_handling\":1}");
}

TEST(Packets, parseCreateHints) {
    json j = json::parse("{\"cmd\":\"CreateHints\",\"locations\":[4,8],\"player\":3}");
    auto packet = static_cast<archipelago::packets::CreateHints>(j);
}

TEST(Packets, encodeCreateHints) {
    archipelago::packets::CreateHints c;
    c.locations = { 6, 7 };
    c.player = 1;
    std::string jsonStr = static_cast<json>(c).dump();
    EXPECT_EQ(jsonStr, "{\"cmd\":\"CreateHints\",\"locations\":[6,7],\"player\":1}");
    c.status = archipelago::packets::HintStatus::HINT_PRIORITY;
    jsonStr = static_cast<json>(c).dump();
    EXPECT_EQ(jsonStr, "{\"cmd\":\"CreateHints\",\"locations\":[6,7],\"player\":1,\"status\":30}");
}

TEST(Packets, parseDataPackage) {
    json j = json::parse("{\"cmd\":\"DataPackage\",\"data\":{\"games\":{\"Some Game\":{\"items_to_ids\":{\"Example\":1}}}}}");
    auto packet = static_cast<archipelago::packets::DataPackage>(j);
}

TEST(Packets, encodeDataPackage) {
    archipelago::packets::DataPackage c;
    c.games.insert({ "Game 1", json::object() });
    std::string jsonStr = static_cast<json>(c).dump();
    EXPECT_EQ(jsonStr, "{\"cmd\":\"DataPackage\",\"data\":{\"games\":{\"Game 1\":{}}}}");
}

TEST(Packets, parseDataStorageOperation) {
    // This one is a bit weirder
    json j = json::parse("{\"operation\":\"default\",\"value\":null}");
    auto op = static_cast<archipelago::packets::DataStorageOperation>(j);
    EXPECT_EQ(op.operation, archipelago::packets::DataStorageOperationType::opDefault);
    EXPECT_THAT(op.value.is_null(), testing::IsTrue());
}

TEST(Packets, encodeDataStorageOperation) {
    archipelago::packets::DataStorageOperation op = {
        .operation = archipelago::packets::DataStorageOperationType::opAdd,
        .value = 7
    };
    std::string jsonStr = static_cast<json>(op).dump();
    EXPECT_EQ(jsonStr, "{\"operation\":\"add\",\"value\":7}");
}

TEST(Packets, parseDataStorageOperationType) {
    // This one is a bit weirder
    json j = json::parse("\"default\"");
    auto op = static_cast<archipelago::packets::DataStorageOperationType>(j);
    EXPECT_EQ(op, archipelago::packets::DataStorageOperationType::opDefault);
}

TEST(Packets, encodeDataStorageOperationType) {
    auto op = archipelago::packets::DataStorageOperationType::opDefault;
    std::string jsonStr = static_cast<json>(op).dump();
    EXPECT_EQ(jsonStr, "\"default\"");
}

TEST(Packets, parseGet) {
    json j = json::parse("{\"cmd\":\"Get\",\"keys\":[\"test_key\"]}");
    auto packet = static_cast<archipelago::packets::Get>(j);
    EXPECT_THAT(packet.keys, testing::ElementsAre("test_key"));
}

TEST(Packets, encodeGet) {
    archipelago::packets::Get c;
    c.keys = { "_read_something", "another_key" };
    std::string jsonStr = static_cast<json>(c).dump();
    EXPECT_EQ(jsonStr, "{\"cmd\":\"Get\",\"keys\":[\"_read_something\",\"another_key\"]}");
}

TEST(Packets, parseGetDataPackage) {
    json j = json::parse("{\"cmd\":\"GetDataPackage\"}");
    auto packet = static_cast<archipelago::packets::GetDataPackage>(j);
}

TEST(Packets, encodeGetDataPackage) {
    archipelago::packets::GetDataPackage c;
    std::string jsonStr = static_cast<json>(c).dump();
    EXPECT_EQ(jsonStr, "{\"cmd\":\"GetDataPackage\"}");
}

TEST(Packets, parseInvalidPacket) {
    json j = json::parse("{\"cmd\":\"InvalidPacket\",\"type\":\"Arguments\",\"text\":\"Retrieve\",\"original_cmd\":\"Set\"}");
    auto packet = static_cast<archipelago::packets::InvalidPacket>(j);
    EXPECT_EQ(packet.type, "Arguments");
    EXPECT_EQ(packet.text, "Retrieve");
    EXPECT_THAT(packet.original_cmd, testing::Optional(std::string("Set")));
}

TEST(Packets, encodeInvalidPacket) {
    archipelago::packets::InvalidPacket p;
    p.type = "Arguments";
    p.text = "Retrieve";
    std::string jsonStr = static_cast<json>(p).dump();
    EXPECT_EQ(jsonStr, "{\"cmd\":\"InvalidPacket\",\"text\":\"Retrieve\",\"type\":\"Arguments\"}");
    p.original_cmd = "Set";
    jsonStr = static_cast<json>(p).dump();
    EXPECT_EQ(jsonStr, "{\"cmd\":\"InvalidPacket\",\"original_cmd\":\"Set\",\"text\":\"Retrieve\",\"type\":\"Arguments\"}");
}

TEST(Packets, parseJSONMessagePart) {
    json j = json::parse("{}");
    auto packet = static_cast<archipelago::packets::JSONMessagePart>(j);
    // Should work and generate an all-nothing packet
    EXPECT_EQ(packet.type, std::nullopt);
    EXPECT_EQ(packet.text, std::nullopt);
    EXPECT_EQ(packet.color, std::nullopt);
    EXPECT_EQ(packet.flags, std::nullopt);
    EXPECT_EQ(packet.player, std::nullopt);
    EXPECT_EQ(packet.hint_status, std::nullopt);
    j = json::parse("{\"color\":\"color\",\"flags\":2,\"hint_status\":10,\"player\":1,\"text\":\"This is some text\",\"type\":\"type\"}");
    packet = static_cast<archipelago::packets::JSONMessagePart>(j);
    // Should work and generate an all-nothing packet
    EXPECT_THAT(packet.type, testing::Optional(std::string("type")));
    EXPECT_THAT(packet.text, testing::Optional(std::string("This is some text")));
    EXPECT_THAT(packet.color, testing::Optional(std::string("color")));
    EXPECT_THAT(packet.flags, testing::Optional(2));
    EXPECT_THAT(packet.player, testing::Optional(1));
    EXPECT_THAT(packet.hint_status, testing::Optional(archipelago::packets::HintStatus::HINT_NO_PRIORITY));
}
TEST(Packets, encodeJSONMessagePart) {
    archipelago::packets::JSONMessagePart p;
    p.type = "type";
    p.text = "This is some text";
    p.color = "color";
    p.flags = 2;
    p.player = 1;
    p.hint_status = archipelago::packets::HintStatus::HINT_NO_PRIORITY;
    std::string jsonStr = static_cast<json>(p).dump();
    EXPECT_EQ(jsonStr, "{\"color\":\"color\",\"flags\":2,\"hint_status\":10,\"player\":1,\"text\":\"This is some text\",\"type\":\"type\"}");
}
TEST(Packets, parseLocationChecks) {
    json j = json::parse("{\"cmd\":\"LocationChecks\"}");
    auto packet = static_cast<archipelago::packets::LocationChecks>(j);
}
TEST(Packets, encodeLocationChecks) {
    archipelago::packets::LocationChecks c;
    std::string jsonStr = static_cast<json>(c).dump();
    EXPECT_EQ(jsonStr, "{\"cmd\":\"LocationChecks\"}");
}
TEST(Packets, parseLocationInfo) {
    json j = json::parse("{\"cmd\":\"LocationInfo\"}");
    auto packet = static_cast<archipelago::packets::LocationInfo>(j);
}
TEST(Packets, encodeLocationInfo) {
    archipelago::packets::LocationInfo c;
    std::string jsonStr = static_cast<json>(c).dump();
    EXPECT_EQ(jsonStr, "{\"cmd\":\"LocationInfo\"}");
}
TEST(Packets, parseLocationScouts) {
    json j = json::parse("{\"cmd\":\"LocationScouts\"}");
    auto packet = static_cast<archipelago::packets::LocationScouts>(j);
}
TEST(Packets, encodeLocationScouts) {
    archipelago::packets::LocationScouts c;
    std::string jsonStr = static_cast<json>(c).dump();
    EXPECT_EQ(jsonStr, "{\"cmd\":\"LocationScouts\"}");
}
TEST(Packets, parseNetworkItem) {
    json j = json::parse("{\"cmd\":\"NetworkItem\"}");
    auto packet = static_cast<archipelago::packets::NetworkItem>(j);
}
TEST(Packets, encodeNetworkItem) {
    archipelago::packets::NetworkItem c;
    std::string jsonStr = static_cast<json>(c).dump();
    EXPECT_EQ(jsonStr, "{\"cmd\":\"NetworkItem\"}");
}
TEST(Packets, parseNetworkPlayer) {
    json j = json::parse("{\"cmd\":\"NetworkPlayer\"}");
    auto packet = static_cast<archipelago::packets::NetworkPlayer>(j);
}
TEST(Packets, encodeNetworkPlayer) {
    archipelago::packets::NetworkPlayer c;
    std::string jsonStr = static_cast<json>(c).dump();
    EXPECT_EQ(jsonStr, "{\"cmd\":\"NetworkPlayer\"}");
}
TEST(Packets, parseNetworkSlot) {
    json j = json::parse("{\"cmd\":\"NetworkSlot\"}");
    auto packet = static_cast<archipelago::packets::NetworkSlot>(j);
}
TEST(Packets, encodeNetworkSlot) {
    archipelago::packets::NetworkSlot c;
    std::string jsonStr = static_cast<json>(c).dump();
    EXPECT_EQ(jsonStr, "{\"cmd\":\"NetworkSlot\"}");
}
TEST(Packets, parseNetworkVersion) {
    json j = json::parse("{\"cmd\":\"NetworkVersion\"}");
    auto packet = static_cast<archipelago::packets::NetworkVersion>(j);
}
TEST(Packets, encodeNetworkVersion) {
    archipelago::packets::NetworkVersion v(0, 6, 7);
    std::string jsonStr = static_cast<json>(v).dump();
    EXPECT_EQ(jsonStr, "{\"build\":7,\"class\":\"Version\",\"major\":0,\"minor\":6}");
}
TEST(Packets, compareNetworkVersion) {
    EXPECT_TRUE(archipelago::packets::NetworkVersion(0, 6, 7) < archipelago::packets::NetworkVersion(0, 6, 9));
    EXPECT_TRUE(archipelago::packets::NetworkVersion(0, 6, 7) < archipelago::packets::NetworkVersion(0, 7, 0));
    EXPECT_TRUE(archipelago::packets::NetworkVersion(0, 6, 7) < archipelago::packets::NetworkVersion(1, 0, 0));
    EXPECT_TRUE(archipelago::packets::NetworkVersion(0, 6, 7) == archipelago::packets::NetworkVersion(0, 6, 7));
}
TEST(Packets, parsePrintJSON) {
    json j = json::parse("{\"cmd\":\"PrintJSON\"}");
    auto packet = static_cast<archipelago::packets::PrintJSON>(j);
}
TEST(Packets, encodePrintJSON) {
    archipelago::packets::PrintJSON c;
    std::string jsonStr = static_cast<json>(c).dump();
    EXPECT_EQ(jsonStr, "{\"cmd\":\"PrintJSON\"}");
}
TEST(Packets, parsePrintJsonType) {
    json j = json::parse("{\"cmd\":\"PrintJsonType\"}");
    auto packet = static_cast<archipelago::packets::PrintJsonType>(j);
}
TEST(Packets, encodePrintJsonType) {
    archipelago::packets::PrintJsonType c;
    std::string jsonStr = static_cast<json>(c).dump();
    EXPECT_EQ(jsonStr, "{\"cmd\":\"PrintJsonType\"}");
}
TEST(Packets, parseReceivedItems) {
    json j = json::parse("{\"cmd\":\"ReceivedItems\"}");
    auto packet = static_cast<archipelago::packets::ReceivedItems>(j);
}
TEST(Packets, encodeReceivedItems) {
    archipelago::packets::ReceivedItems c;
    std::string jsonStr = static_cast<json>(c).dump();
    EXPECT_EQ(jsonStr, "{\"cmd\":\"ReceivedItems\"}");
}
TEST(Packets, parseRetrieved) {
    json j = json::parse("{\"cmd\":\"Retrieved\"}");
    auto packet = static_cast<archipelago::packets::Retrieved>(j);
}
TEST(Packets, encodeRetrieved) {
    archipelago::packets::Retrieved c;
    std::string jsonStr = static_cast<json>(c).dump();
    EXPECT_EQ(jsonStr, "{\"cmd\":\"Retrieved\"}");
}
TEST(Packets, parseRoomInfo) {
    json j = json::parse("{\"cmd\":\"RoomInfo\"}");
    auto packet = static_cast<archipelago::packets::RoomInfo>(j);
}
TEST(Packets, encodeRoomInfo) {
    archipelago::packets::RoomInfo c;
    std::string jsonStr = static_cast<json>(c).dump();
    EXPECT_EQ(jsonStr, "{\"cmd\":\"RoomInfo\"}");
}
TEST(Packets, parseRoomUpdate) {
    json j = json::parse("{\"cmd\":\"RoomUpdate\"}");
    auto packet = static_cast<archipelago::packets::RoomUpdate>(j);
}
TEST(Packets, encodeRoomUpdate) {
    archipelago::packets::RoomUpdate c;
    std::string jsonStr = static_cast<json>(c).dump();
    EXPECT_EQ(jsonStr, "{\"cmd\":\"RoomUpdate\"}");
}
TEST(Packets, parseSay) {
    json j = json::parse("{\"cmd\":\"Say\"}");
    auto packet = static_cast<archipelago::packets::Say>(j);
}
TEST(Packets, encodeSay) {
    archipelago::packets::Say c;
    std::string jsonStr = static_cast<json>(c).dump();
    EXPECT_EQ(jsonStr, "{\"cmd\":\"Say\"}");
}
TEST(Packets, parseSet) {
    json j = json::parse("{\"cmd\":\"Set\"}");
    auto packet = static_cast<archipelago::packets::Set>(j);
}
TEST(Packets, encodeSet) {
    archipelago::packets::Set c;
    std::string jsonStr = static_cast<json>(c).dump();
    EXPECT_EQ(jsonStr, "{\"cmd\":\"Set\"}");
}
TEST(Packets, parseSetNotify) {
    json j = json::parse("{\"cmd\":\"SetNotify\"}");
    auto packet = static_cast<archipelago::packets::SetNotify>(j);
}
TEST(Packets, encodeSetNotify) {
    archipelago::packets::SetNotify c;
    std::string jsonStr = static_cast<json>(c).dump();
    EXPECT_EQ(jsonStr, "{\"cmd\":\"SetNotify\"}");
}
TEST(Packets, parseSetReply) {
    json j = json::parse("{\"cmd\":\"SetReply\"}");
    auto packet = static_cast<archipelago::packets::SetReply>(j);
}
TEST(Packets, encodeSetReply) {
    archipelago::packets::SetReply c;
    std::string jsonStr = static_cast<json>(c).dump();
    EXPECT_EQ(jsonStr, "{\"cmd\":\"SetReply\"}");
}
TEST(Packets, parseSlotInfo) {
    json j = json::parse("{\"cmd\":\"SlotInfo\"}");
    auto packet = static_cast<archipelago::packets::SlotInfo>(j);
}
TEST(Packets, encodeSlotInfo) {
    archipelago::packets::SlotInfo c;
    std::string jsonStr = static_cast<json>(c).dump();
    EXPECT_EQ(jsonStr, "{\"cmd\":\"SlotInfo\"}");
}
TEST(Packets, parseStatusUpdate) {
    json j = json::parse("{\"cmd\":\"StatusUpdate\"}");
    auto packet = static_cast<archipelago::packets::StatusUpdate>(j);
}
TEST(Packets, encodeStatusUpdate) {
    archipelago::packets::StatusUpdate c;
    std::string jsonStr = static_cast<json>(c).dump();
    EXPECT_EQ(jsonStr, "{\"cmd\":\"StatusUpdate\"}");
}
TEST(Packets, parseSync) {
    json j = json::parse("{\"cmd\":\"Sync\"}");
    auto packet = static_cast<archipelago::packets::Sync>(j);
}
TEST(Packets, encodeSync) {
    archipelago::packets::Sync c;
    std::string jsonStr = static_cast<json>(c).dump();
    EXPECT_EQ(jsonStr, "{\"cmd\":\"Sync\"}");
}
TEST(Packets, parseUpdateHint) {
    json j = json::parse("{\"cmd\":\"UpdateHint\"}");
    auto packet = static_cast<archipelago::packets::UpdateHint>(j);
}
TEST(Packets, encodeUpdateHint) {
    archipelago::packets::UpdateHint c;
    std::string jsonStr = static_cast<json>(c).dump();
    EXPECT_EQ(jsonStr, "{\"cmd\":\"UpdateHint\"}");
}

