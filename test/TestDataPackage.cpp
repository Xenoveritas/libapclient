/*
 * Test file for ensuring data package cache works.
 */

#include "libapclient/data_package.h"
#include "libapclient/logger.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

TEST(DataPackage, testEmpty) {
    archipelago::GameDataCache cache;
    EXPECT_EQ(cache.size(), 0);
    EXPECT_THAT(cache.findMissingGames(), testing::IsEmpty());
    EXPECT_THAT(cache.getGames(), testing::IsEmpty());
    EXPECT_EQ(cache.getChecksum("Anything"), nullptr);
}

TEST(DataPackage, testCheckRoomInfo) {
    archipelago::GameDataCache cache;
    archipelago::packets::RoomInfo roomInfo{
        .datapackage_checksums = {
            { "Test Game", "checksum1" },
            { "Other Game", "checksum2" }
        }
    };
    cache.checkRoomInfo(roomInfo);
    // Cache should now have two games in it but no game data
    EXPECT_EQ(cache.size(), 0);
    EXPECT_EQ(cache.knownChecksumCount(), 2);
    EXPECT_THAT(cache.getChecksum("Test Game"), testing::Pointee(std::string("checksum1")));
    EXPECT_THAT(cache.getChecksum("Other Game"), testing::Pointee(std::string("checksum2")));
    // Should have two missing games (order is not maintained)
    EXPECT_THAT(cache.findMissingGames(), testing::UnorderedElementsAre("Other Game", "Test Game"));
}

TEST(IDMap, testAPIs) {
    archipelago::IDMap<int, std::string> idMap;
    std::string obj1{"Test 1"}, obj2{"Another object"};
    EXPECT_TRUE(idMap.empty());
    idMap.insert(1, obj1);
    EXPECT_FALSE(idMap.empty());
    idMap.insert(3, obj2);
    EXPECT_EQ(idMap.getById(4), nullptr);
    const std::string* actual = idMap.getById(1);
    // should be copied on insert
    EXPECT_EQ(*actual, obj1);
    EXPECT_EQ(idMap.getByObject(std::string("Other object")), std::nullopt);
    EXPECT_THAT(idMap.getByObject(obj1), testing::Optional(1));
    EXPECT_THAT(idMap.getIDs(), testing::ElementsAre(1, 3));
    EXPECT_THAT(idMap.getObjects(), testing::ElementsAre(obj1, obj2));
}

TEST(GameData, testParse) {
    json j = json::parse("{\"item_name_to_id\":{\"First Item\":1,\"Second Item\":2},\"location_name_to_id\":{\"First Location\":10,\"Second Location\":20}}");
    archipelago::GameData data;
    data << j;
    EXPECT_EQ(data.getItemName(1), "First Item");
    EXPECT_EQ(data.getItemName(2), "Second Item");
    EXPECT_EQ(data.getItemName(3), "Item<3>");
    EXPECT_THAT(data.getItemID("First Item"), testing::Optional(1));
    EXPECT_THAT(data.getItemID("Second Item"), testing::Optional(2));
    EXPECT_EQ(data.getItemID("Third Item"), std::nullopt);
    EXPECT_FALSE(data.hasItemId(0));
    EXPECT_TRUE(data.hasItemId(1));
    EXPECT_TRUE(data.hasItemId(2));
    EXPECT_FALSE(data.hasItemId(3));
    EXPECT_EQ(data.getLocationName(10), "First Location");
    EXPECT_EQ(data.getLocationName(20), "Second Location");
    EXPECT_EQ(data.getLocationName(30), "Location<30>");
    EXPECT_FALSE(data.hasLocationId(0));
    EXPECT_TRUE(data.hasLocationId(10));
    EXPECT_TRUE(data.hasLocationId(20));
    EXPECT_FALSE(data.hasLocationId(3));
    EXPECT_THAT(data.getLocationID("First Location"), testing::Optional(10));
    EXPECT_THAT(data.getLocationID("Second Location"), testing::Optional(20));
    EXPECT_EQ(data.getLocationID("Third Location"), std::nullopt);
}