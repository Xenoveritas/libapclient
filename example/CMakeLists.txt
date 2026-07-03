# Demo executable

# The client demo uses FTXUI for the terminal UI.

if (NOT TARGET ftxui::ftxui)
    CPMAddPackage(
        NAME FTXUI
        GITHUB_REPOSITORY ArthurSonzogni/FTXUI
        VERSION 7.0.0
    )
endif()

add_executable(apclient "apclient.cpp" "commands.cpp" "commands.h")
target_link_libraries(apclient PRIVATE libapclient)
target_link_libraries(apclient PRIVATE ftxui::ftxui)

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET apclient PROPERTY CXX_STANDARD 20)
endif()
