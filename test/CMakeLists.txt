# Including this file means tests are enabled, period

if (NOT TARGET GTest::gtest_main)
    set(gtest_force_shared_crt on)
    CPMAddPackage(
        NAME GoogleTest
        GITHUB_REPOSITORY google/googletest
        GIT_TAG v1.17.0
        VERSION 1.17.0
        SYSTEM ON
    )
endif()

enable_testing()

set (TEST_SOURCES
    "TestPackets.cpp"
)

add_executable(libapclient_test ${TEST_SOURCES})
target_link_libraries(libapclient_test PRIVATE libapclient)
target_link_libraries(libapclient_test PRIVATE GTest::gtest_main)
target_link_libraries(libapclient_test PRIVATE GTest::gmock_main)
set_property(TARGET libapclient_test PROPERTY CXX_STANDARD 20)
