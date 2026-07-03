/*! \file commands.cpp
 */

#include "commands.h"

namespace command {

void uuid(archipelago::SimpleClient& client, const std::vector<std::string>& args) {
    bool create = args.size() >= 2 && args[1] == "create";
    try {
        auto uuid = archipelago::getCachedPlayerUUID();
        if (uuid.has_value()) {
            client.write("Your client UUID is { ");
            client.write(*uuid);
            client.writeLn(" }.");
        } else {
            if (create) {
                client.writeLn("No cached client UUID exists.");
                client.write("Created a new UUID { ");
                client.write(archipelago::getPlayerUUID(true));
                client.writeLn(" }.");
            }
            else {
                client.writeLn("Could not look up your client UUID. (You may not have an Archipelago-generated client ID cached yet.)", archipelago::MessageType::error);
            }
        }
    } catch (const std::runtime_error& e) {
        client.write("Unable to look up your UUID: ", archipelago::MessageType::error);
        client.writeLn(e.what(), archipelago::MessageType::error);
    }
}

void cacheDir(archipelago::SimpleClient& client, const std::vector<std::string>& args) {
    try {
        auto path = archipelago::getPlayerArchipelagoCacheDirectory();
        client.write("Archipelago cache directory: ");
        client.writeLn(path.string());
    } catch (const std::runtime_error& e) {
        client.write("Unable to look up the cache directory: ", archipelago::MessageType::error);
        client.writeLn(e.what(), archipelago::MessageType::error);
    }
}

void say(archipelago::SimpleClient& client, const std::vector<std::string>& args) {
    // This is similar to the default but goes through the tokenizer
    std::string message;
    size_t length = 0;
    // need to skip the first element
    for (auto it = args.cbegin() + 1; it != args.cend(); ++it) {
        length += (*it).size();
        // and the space between them
        length++;
    }
    if (length <= 1) {
        // If no arguments, don't send an empty packet.
        return;
    }
    message.reserve(length);
    for (auto it = args.cbegin() + 1; it != args.cend(); ++it) {
        message.append(*it);
        message.push_back(' ');
    }
    // Just trim off the final space
    message.resize(message.size() - 1);
    try {
        client.sendSay(message);
    } catch (const archipelago::InvalidStateError&) {
        client.writeLn("Cannot send chat messages until the client is connected.", archipelago::MessageType::error);
    }
}

}