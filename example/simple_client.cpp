/*! \file simple_client.cpp
 * \brief Archipelago demo client
 *
 */

#include "libapclient/libapclient.h"
// Currently not part libapclient.h but may become part
#include "libapclient/simple_client.h"
#include "libapclient/tokenizer.h"
#include "libapclient/data_package.h"
#include "libapclient/tracker.h"
#include "libapclient/logger.h"

#include <iostream>

// Tell Windows not to break everything.
#define NOMINMAX
#include <ixwebsocket/IXNetSystem.h>

int main(int argc, char* argv[]) {
    // Required under Windows
    ix::initNetSystem();

    archipelago::TerminalClient app{std::cin, std::cout};
    app.run();

    return 0;
}
