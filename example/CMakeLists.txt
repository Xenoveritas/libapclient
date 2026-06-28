# Demo executable
add_executable(apclient "terminal_client.cpp" "tokenizer.cpp" "tokenizer.h")
target_link_libraries(apclient PRIVATE libapclient)

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET apclient PROPERTY CXX_STANDARD 20)
endif()
