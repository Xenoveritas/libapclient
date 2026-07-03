/*! \file apclient.cpp
 * \brief Archipelago demo client
 *
 */

#include "libapclient/libapclient.h"
// Currently not part libapclient.h but may become part
#include "libapclient/simple_client.h"
#include "libapclient/tokenizer.h"
#include "libapclient/tracker.h"

#include <iostream>
#include <stdexcept>

#include <ixwebsocket/IXNetSystem.h>

class APClient : public archipelago::TerminalClient {
public:
    APClient() : archipelago::TerminalClient(std::cin, std::cout) {}

    void writeStatus() {
        std::lock_guard lock(m_mutex);
        write("Websocket: ");
        if (m_socket == nullptr) {
            writeLn("not connected");
        } else {
            switch (m_socket->getReadyState()) {
            case ix::ReadyState::Connecting:
                writeLn("connecting");
                break;
            case ix::ReadyState::Open:
                writeLn("open");
                break;
            case ix::ReadyState::Closing:
                writeLn("closing");
                break;
            case ix::ReadyState::Closed:
                writeLn("closed");
                break;
            }
        }
        write("Client state: ");
        switch (getUnlockedState()) {
        case archipelago::ClientState::disconnected:
            writeLn("disconnected");
            break;
        case archipelago::ClientState::disconnecting:
            writeLn("disconnecting");
            break;
        case archipelago::ClientState::connecting:
            writeLn("connecting (no response from server yet)");
            break;
        case archipelago::ClientState::websocketConnected:
            writeLn("websocket connected (waiting for Archipelago start)");
            break;
        case archipelago::ClientState::receivedRoomInfo:
            writeLn("received room info, response not sent");
            break;
        case archipelago::ClientState::sentGetDataPackage:
            writeLn("requested data package, waiting for data");
            break;
        case archipelago::ClientState::receivedDataPackage:
            writeLn("received data package, response not sent");
            break;
        case archipelago::ClientState::sentConnect:
            writeLn("sent connection request");
            break;
        case archipelago::ClientState::connected:
            writeLn("connected");
            break;
        case archipelago::ClientState::connectionRefused:
            writeLn("websocket connected, active connection refused");
            break;
        default:
            writeLn("internal error");
        }
    }
};

// Command implementations

void showUUID(archipelago::SimpleClient& client, const std::vector<std::string>& args) {
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

void showCacheDir(archipelago::SimpleClient& client, const std::vector<std::string>& args) {
    try {
        auto path = archipelago::getPlayerArchipelagoCacheDirectory();
        client.write("Archipelago cache directory: ");
        client.writeLn(path.string());
    } catch (const std::runtime_error& e) {
        client.write("Unable to look up the cache directory: ", archipelago::MessageType::error);
        client.writeLn(e.what(), archipelago::MessageType::error);
    }
}

void commandStatus(archipelago::SimpleClient& client, const std::vector<std::string>& args) {
    reinterpret_cast<APClient&>(client).writeStatus();
}

void commandSay(archipelago::SimpleClient& client, const std::vector<std::string>& args) {
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

int main(int argc, char* argv[]) {
    // Required under Windows
    ix::initNetSystem();

    std::cout << "Welcome to AP Client!" << std::endl;
    std::cout << "Use \"/connect <server> <player> [password]\" to connect or \"help\" to display additional commands." << std::endl;
    auto app = archipelago::TerminalClient(std::cin, std::cout);
    app.addCommand("status", commandStatus, "shows current client status");
    app.addCommand("uuid", showUUID, "display your client UUID", "[create]", "Display the cached Archipelago UUID if it exists.\nIf it does not exist and \"create\" is given, a new UUID will be generated.");
    app.addCommand("cachedir", showCacheDir);
    app.run();

    return 0;
}
