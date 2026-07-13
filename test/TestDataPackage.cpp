/*
 * Test file for ensuring data package cache works.
 */

#include "libapclient/data_package.h"
#include "libapclient/logger.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

TEST(DataPackage, testEmpty) {
    archipelago::DataPackageCache<> cache;
    EXPECT_EQ(cache.size(), 0);
    EXPECT_THAT(cache.findMissingGames(), testing::IsEmpty());
    EXPECT_THAT(cache.getGames(), testing::IsEmpty());
    EXPECT_EQ(cache.getChecksum("Anything"), nullptr);
}
