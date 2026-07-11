/*! \file commands.h
 * \brief Basic commands that work with any archipelago::SimpleClient
 */

#include "libapclient/simple_client.h"

namespace command {

void uuid(archipelago::SimpleClient& client, const std::vector<std::string>& args);

void cacheDir(archipelago::SimpleClient& client, const std::vector<std::string>& args);

void say(archipelago::SimpleClient& client, const std::vector<std::string>& args);

void get(archipelago::SimpleClient& client, const std::vector<std::string>& args);

}