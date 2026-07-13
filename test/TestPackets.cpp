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
    b.games = { "super test", "super test ultra deluxe edition" };
    b.slots = { 42, 67 };
    b.tags = { "example" };
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
    json j = json::parse("{\"cmd\":\"LocationChecks\",\"locations\":[1,2,3,5,7]}");
    auto packet = static_cast<archipelago::packets::LocationChecks>(j);
    EXPECT_THAT(packet.locations, testing::ElementsAre(1, 2, 3, 5, 7));
}
TEST(Packets, encodeLocationChecks) {
    archipelago::packets::LocationChecks checks;
    checks.locations = { 1, 3, 5 };
    std::string jsonStr = static_cast<json>(checks).dump();
    EXPECT_EQ(jsonStr, "{\"cmd\":\"LocationChecks\",\"locations\":[1,3,5]}");
}
TEST(Packets, parseLocationInfo) {
    // The bulk of this is in NetworkItem
    json j = json::parse("{\"cmd\":\"LocationInfo\",\"locations\":[]}");
    auto packet = static_cast<archipelago::packets::LocationInfo>(j);
    EXPECT_THAT(packet.locations, testing::IsEmpty());
}
TEST(Packets, encodeLocationInfo) {
    archipelago::packets::LocationInfo locationInfo;
    locationInfo.locations = { archipelago::packets::NetworkItem() };
    std::string jsonStr = static_cast<json>(locationInfo).dump();
    EXPECT_EQ(jsonStr, "{\"cmd\":\"LocationInfo\",\"locations\":[{\"class\":\"NetworkItem\",\"flags\":0,\"item\":0,\"location\":0,\"player\":0}]}");
}
TEST(Packets, parseLocationScouts) {
    json j = json::parse("{\"cmd\":\"LocationScouts\",\"locations\":[101,102],\"create_as_hint\":0}");
    auto packet = static_cast<archipelago::packets::LocationScouts>(j);
    EXPECT_THAT(packet.locations, testing::ElementsAre(101, 102));
    EXPECT_EQ(packet.create_as_hint, 0);
}
TEST(Packets, encodeLocationScouts) {
    archipelago::packets::LocationScouts scouts;
    scouts.locations = { 100 };
    scouts.create_as_hint = 1;
    std::string jsonStr = static_cast<json>(scouts).dump();
    EXPECT_EQ(jsonStr, "{\"cmd\":\"LocationScouts\",\"create_as_hint\":1,\"locations\":[100]}");
}
TEST(Packets, parseNetworkItem) {
    json j = json::parse("{\"flags\":0,\"item\":12,\"location\":45,\"player\":3}");
    auto packet = static_cast<archipelago::packets::NetworkItem>(j);
    EXPECT_EQ(packet.flags, 0);
    EXPECT_EQ(packet.item, 12);
    EXPECT_EQ(packet.location, 45);
    EXPECT_EQ(packet.player, 3);
}
TEST(Packets, encodeNetworkItem) {
    archipelago::packets::NetworkItem networkItem = {
        .item = 6,
        .location = 7,
        .player = 8,
        .flags = 1
    };
    std::string jsonStr = static_cast<json>(networkItem).dump();
    EXPECT_EQ(jsonStr, "{\"class\":\"NetworkItem\",\"flags\":1,\"item\":6,\"location\":7,\"player\":8}");
}
TEST(Packets, parseNetworkPlayer) {
    json j = json::parse("{\"team\":1,\"slot\":2,\"alias\":\"Alias\",\"name\":\"Slot\"}");
    auto packet = static_cast<archipelago::packets::NetworkPlayer>(j);
    EXPECT_EQ(packet.alias, "Alias");
    EXPECT_EQ(packet.name, "Slot");
    EXPECT_EQ(packet.team, 1);
    EXPECT_EQ(packet.slot, 2);
}
TEST(Packets, encodeNetworkPlayer) {
    archipelago::packets::NetworkPlayer player = {
        .team = 1,
        .slot = 3,
        .alias = "Alias",
        .name = "Name"
    };
    std::string jsonStr = static_cast<json>(player).dump();
    EXPECT_EQ(jsonStr, "{\"alias\":\"Alias\",\"class\":\"NetworkPlayer\",\"name\":\"Name\",\"slot\":3,\"team\":1}");
}
TEST(Packets, parseNetworkSlot) {
    json j = json::parse("{\"class\":\"NetworkSlot\",\"name\":\"Player\",\"game\":\"GoogleTest\",\"type\":1,\"group_members\":[]}");
    auto packet = static_cast<archipelago::packets::NetworkSlot>(j);
    EXPECT_EQ(packet.name, "Player");
    EXPECT_EQ(packet.game, "GoogleTest");
    EXPECT_EQ(packet.type, 1);
    EXPECT_THAT(packet.group_members, testing::IsEmpty());
}
TEST(Packets, encodeNetworkSlot) {
    archipelago::packets::NetworkSlot slot = {
        .name = "Name",
        .game = "GoogleTest",
        .type = archipelago::packets::SlotType::player,
        .group_members = {}
    };
    std::string jsonStr = static_cast<json>(slot).dump();
    EXPECT_EQ(jsonStr, "{\"class\":\"NetworkSlot\",\"game\":\"GoogleTest\",\"group_members\":[],\"name\":\"Name\",\"type\":1}");
}
TEST(Packets, parseNetworkVersion) {
    json j = json::parse("{\"class\":\"Version\",\"major\":1,\"minor\":0,\"build\":2}");
    auto packet = static_cast<archipelago::packets::NetworkVersion>(j);
    EXPECT_EQ(packet.major, 1);
    EXPECT_EQ(packet.minor, 0);
    EXPECT_EQ(packet.build, 2);
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
    json j = json::parse("{\"cmd\":\"PrintJSON\",\"data\":[]}");
    auto packet = static_cast<archipelago::packets::PrintJSON>(j);
    EXPECT_THAT(packet.data, testing::IsEmpty());
    EXPECT_EQ(packet.type, archipelago::packets::PrintJsonType::none);
    EXPECT_EQ(packet.receiving, std::nullopt);
    EXPECT_EQ(packet.item, std::nullopt);
    EXPECT_EQ(packet.found, std::nullopt);
    EXPECT_EQ(packet.team, std::nullopt);
    EXPECT_EQ(packet.slot, std::nullopt);
    EXPECT_EQ(packet.message, std::nullopt);
    EXPECT_EQ(packet.tags, std::nullopt);
    EXPECT_EQ(packet.countdown, std::nullopt);
    // TODO: Actually test more of that
}
TEST(Packets, encodePrintJSON) {
    archipelago::packets::PrintJSON c;
    std::string jsonStr = static_cast<json>(c).dump();
    EXPECT_EQ(jsonStr, "{\"cmd\":\"PrintJSON\",\"data\":[]}");
    // TODO: Actually test more of that
}
TEST(Packets, parsePrintJsonType) {
    json j = json::parse("\"Something That Doesn't Exist\"");
    auto type = static_cast<archipelago::packets::PrintJsonType>(j);
    EXPECT_EQ(type, archipelago::packets::PrintJsonType::unknown);
    j = json::parse("\"Chat\"");
    type = static_cast<archipelago::packets::PrintJsonType>(j);
    EXPECT_EQ(type, archipelago::packets::PrintJsonType::Chat);
}
TEST(Packets, encodePrintJsonType) {
    archipelago::packets::PrintJsonType type;
    std::string jsonStr = static_cast<json>(archipelago::packets::PrintJsonType::Chat).dump();
    EXPECT_EQ(jsonStr, "\"Chat\"");
}
TEST(Packets, parseReceivedItems) {
    json j = json::parse("{\"cmd\":\"ReceivedItems\",\"index\":0,\"items\":[]}");
    auto packet = static_cast<archipelago::packets::ReceivedItems>(j);
    EXPECT_EQ(packet.index, 0);
    EXPECT_THAT(packet.items, testing::IsEmpty());
}
TEST(Packets, encodeReceivedItems) {
    archipelago::packets::ReceivedItems items;
    items.index = 1;
    items.items = { archipelago::packets::NetworkItem({.item = 1, .location = 12, .player = 2, .flags = 0}) };
    std::string jsonStr = static_cast<json>(items).dump();
    EXPECT_EQ(jsonStr, "{\"cmd\":\"ReceivedItems\",\"index\":1,\"items\":[{\"class\":\"NetworkItem\",\"flags\":0,\"item\":1,\"location\":12,\"player\":2}]}");
}
TEST(Packets, parseRetrieved) {
    json j = json::parse("{\"cmd\":\"Retrieved\",\"keys\":{\"unknown\":\"this is probably wrong\"}}");
    auto packet = static_cast<archipelago::packets::Retrieved>(j);
}
TEST(Packets, encodeRetrieved) {
    archipelago::packets::Retrieved c;
    std::string jsonStr = static_cast<json>(c).dump();
    EXPECT_EQ(jsonStr, "{\"cmd\":\"Retrieved\",\"keys\":{}}");
}
TEST(Packets, parseRoomInfo) {
    json j = json::parse("{\"cmd\":\"RoomInfo\",\"datapackage_checksums\":{},\"games\":[],\"generator_version\":{\"build\":0,\"class\":\"Version\",\"major\":0,\"minor\":0},\"hint_cost\":0,\"location_check_points\":0,\"password\":false,\"permissions\":{},\"seed_name\":\"\",\"tags\":[],\"time\":0.0,\"version\":{\"build\":0,\"class\":\"Version\",\"major\":0,\"minor\":0}}");
    auto packet = static_cast<archipelago::packets::RoomInfo>(j);
}
TEST(Packets, encodeRoomInfo) {
    archipelago::packets::RoomInfo c;
    std::string jsonStr = static_cast<json>(c).dump();
    EXPECT_EQ(jsonStr, "{\"cmd\":\"RoomInfo\",\"datapackage_checksums\":{},\"games\":[],\"generator_version\":{\"build\":0,\"class\":\"Version\",\"major\":0,\"minor\":0},\"hint_cost\":0,\"location_check_points\":0,\"password\":false,\"permissions\":{},\"seed_name\":\"\",\"tags\":[],\"time\":0.0,\"version\":{\"build\":0,\"class\":\"Version\",\"major\":0,\"minor\":0}}");
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

