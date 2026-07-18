# Demo executable

# The client demo uses FTXUI for the terminal UI.

if (NOT TARGET ftxui::ftxui)
    CPMAddPackage(
        NAME FTXUI
        GITHUB_REPOSITORY ArthurSonzogni/FTXUI
        VERSION 7.0.0
    )
endif()

add_executable(apclient
    "apclient.cpp"
    "commands.cpp" "commands.h"
    "console_component.cpp" "console_component.h"
)
target_link_libraries(apclient PRIVATE libapclient)
target_link_libraries(apclient PRIVATE ftxui::ftxui)

# Demo very simple client (basic terminal, nothing fancy)
add_executable(simpleclient
    "simple_client.cpp"
)
target_link_libraries(simpleclient PRIVATE libapclient)

set_property(TARGET apclient PROPERTY CXX_STANDARD 20)
set_property(TARGET simpleclient PROPERTY CXX_STANDARD 20)
