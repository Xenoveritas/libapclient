cmake_minimum_required (VERSION 3.14)

# Enable Hot Reload for MSVC compilers if supported.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

cmake_policy(SET CMP0048 NEW)

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
set(BUILD_SHARED_LIBS OFF)

include(CPM)

project ("libapclient"
    VERSION 0.0.1
    DESCRIPTION "Archipelago client library"
    SPDX_LICENSE MIT
    LANGUAGES CXX
)

option(LIBAPCLIENT_SHARED "Compile libapclient as a shared library." OFF)
option(LIBAPCLIENT_BUILD_EXAMPLE "Build the example Archipelago client program." OFF)
# TLS will be required before this is usable. This is currently set to off to avoid having to configure that.
option(LIBAPCLIENT_USE_TLS "Whether to enable TLS." OFF)

# Add dependencies. Because this is intended to be used as a library with projects that
# very well may use the same libraries (especially zlib), check if the targets already
# exist and then just skip the CPMAddPackage if they do under the assumption that
# the parent project knows what they're doing.
if (NOT TARGET nlohmann_json)
    CPMAddPackage(
        NAME nlohmann_json
        GITHUB_REPOSITORY nlohmann/json
        GIT_TAG v3.12.0
        VERSION 3.12.0
        SYSTEM ON
    )
endif()
if (NOT TARGET zlibstatic)
    # IXWebSocket requires ZLib, configure it to be statically linked
    CPMAddPackage(
        NAME ZLIB
        GITHUB_REPOSITORY madler/zlib
        GIT_TAG v1.3.2
        VERSION 1.3.1
        OPTIONS
            "ZLIB_BUILD_TESTING OFF"
            "ZLIB_BUILD_SHARED OFF"
            "ZLIB_BUILD_STATIC ON"
            "ZLIB_INSTALL OFF"
    )
    add_library(ZLIB::ZLIB ALIAS zlibstatic)
endif()
# Try to force find_package to find the CPM-provided version of ZLib
SET(ZLIB_DIR "${ZLIB_BINARY_DIR}" CACHE PATH "" FORCE)
SET(ZLIB_LIBRARY "${ZLIB_BINARY_DIR}" CACHE PATH "" FORCE)
SET(ZLIB_INCLUDE_DIR "${ZLIB_BINARY_DIR}" CACHE PATH "" FORCE)
if (NOT TARGET ixwebsocket)
    CPMAddPackage(
        NAME ixwebsocket
        GITHUB_REPOSITORY machinezone/IXWebSocket
        GIT_TAG v12.0.0
        VERSION 12.0.0
        OPTIONS
            "USE_TLS ${LIBAPCLIENT_USE_TLS}"
            "IXWEBSOCKET_INSTALL OFF"
    )
endif()

# Archipelago wants a UUID for each user, this library is used to generate one if necessary
if (NOT TARGET stduuid)
    CPMAddPackage("gh:mariusbancila/stduuid@1.2.3")
endif()

set(LIBAPCLIENT_SOURCES
    "src/libapclient.cpp"
    "src/logger.cpp"
    "src/packets.cpp"
    "src/simple_client.cpp"
    "src/tracker.cpp"
    "src/utils.cpp"
)

set(LIBAPCLIENT_HEADERS
    "include/libapclient/libapclient.h"
    "include/libapclient/logger.h"
    "include/libapclient/packets.h"
    "include/libapclient/simple_client.h"
    "include/libapclient/tokenizer.h"
    "include/libapclient/tracker.h"
    "include/libapclient/utils.h"
)

if (${LIBAPCLIENT_SHARED})
    set(LIBAPCLIENT_LINK_MODE "SHARED")
else()
    set(LIBAPCLIENT_LINK_MODE "STATIC")
endif()

add_library(libapclient ${LIBAPCLIENT_LINK_MODE} ${LIBAPCLIENT_SOURCES} ${LIBAPCLIENT_HEADERS})

target_compile_definitions(libapclient PUBLIC "-DLIBAPCLIENT_ENABLE_LOGGING")
target_include_directories(libapclient PUBLIC "include")
target_link_libraries(libapclient PUBLIC nlohmann_json::nlohmann_json)
target_link_libraries(libapclient PUBLIC ixwebsocket::ixwebsocket)
target_link_libraries(libapclient PUBLIC stduuid)

find_package(Doxygen)
if(Doxygen_FOUND)
    set(DOXYGEN_USE_MDFILE_AS_MAINPAGE "README.md")
    doxygen_add_docs(docs
        "README.md" ${LIBAPCLIENT_SOURCES} ${LIBAPCLIENT_HEADERS}
    )
else()
    message(STATUS "Doxygen not found, documentation target will not be generated.")
endif()

# On macOS this requires CoreFoundation (both for looking up the cache directory
# and generating a client UUID if necessary)
if (APPLE)
    find_library(CORE_FOUNDATION CoreFoundation)
    target_link_libraries(libapclient PUBLIC ${CORE_FOUNDATION})
endif()

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET libapclient PROPERTY CXX_STANDARD 20)
endif()

if (${LIBAPCLIENT_BUILD_EXAMPLE})
    add_subdirectory(example)
endif()
