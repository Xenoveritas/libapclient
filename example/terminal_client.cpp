#include "libapclient/libapclient.h"
// Currently not part libapclient.h but may become part
#include "libapclient/simple_client.h"
#include "libapclient/tokenizer.h"
#include "libapclient/tracker.h"

#include <iostream>
#include <stdexcept>

#include <ixwebsocket/IXNetSystem.h>


/*
* Default client commands:
/license
    Returns the licensing information
/exit
    Close connections and client
/connect [address]
    Connect to a MultiWorld Server
/disconnect
    Disconnect from a MultiWorld Server
/received
    List all received items
/missing [filter_text]
    List all missing location checks, from your local game state.
    Can be given text, which will be used as filter.
/items
    List all item names for the currently running game.
/locations
    List all location names for the currently running game.
/item_groups [key]
    List all item group names for the currently running game.

    :param key: Which item group to filter to. Will log all groups if empty.
/location_groups [key]
    List all location group names for the currently running game.

    :param key: Which item group to filter to. Will log all groups if empty.
/ready
    Send ready status to server.
*/

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

// Most of this is now moved into the library and this is being slowly dismantled
class OldTerminalClient : public archipelago::TrackerClient {
private:
    std::istream& m_in;
    std::ostream& m_out;
    std::optional<std::string> m_gameName;
    archipelago::LocationTracker<bool> m_locations;
public:
    OldTerminalClient(std::istream& in = std::cin, std::ostream& out = std::cout) : archipelago::TrackerClient(), m_in(in), m_out(out) {}

    // Request item info
    void createConnect(archipelago::packets::Connect& connect) override {
        // For this, just grab everything
        connect.items_handling = archipelago::packets::ItemsHandling::all;
        if (m_gameName.has_value()) {
            connect.game = *m_gameName;
        } else {
            // Only send the TextOnly tag when connecting as a game client
            connect.tags.push_back(archipelago::tag::kTextOnly);
        }
    }

    void onConnected(const archipelago::packets::Connected& connected) override {
        archipelago::TrackerClient::onConnected(connected);
        m_locations << connected;
    }

    void run() {
        m_out << "Welcome to APClient! Use /connect to connect." << std::endl;
        do {
            m_out << "> " << std::flush;
        } while (readLine());
    }

    bool readLine() {
        std::string str;
        if (!std::getline(m_in, str)) {
            return false;
        }
        // If the user didn't enter anything, don't do anything.
        if (str.empty()) {
            return true;
        }
        // parse the string - ish. Command must start with a '/'
        if (str.front() == '/') {
            std::vector<std::string> tokens;
            archipelago::tokenize(str, tokens);
            if (!tokens.empty()) {
                std::string& command = tokens.front();
                if (command == "/connect") {
                    // pass off to connect
                    commandConnect(tokens);
                } else if (command == "/exit" || command == "/quit") {
                    // quit and exit terminate the client
                    return false;
                } else if (command == "/uuid") {
                    commandUUID(tokens);
                } else if (command == "/cachedir") {
                    commandCacheDir(tokens);
                } else if (command == "/disconnect") {
                    m_out << "Disconnecting..." << std::endl;
                    disconnect();
                } else if (command == "/say") {
                    commandSay(tokens);
                } else if (command == "/status") {
                    commandStatus(tokens);
                } else if (command == "/game") {
                    commandGame(tokens);
                } else if (command == "/tracker") {
                    commandTracker(tokens);
                } else if (command == "/release_location") {
                    commandReleaseLocation(tokens);
                } else if (command == "/help") {
                    commandHelp(tokens);
                } else if (command == "/echo") {
                    for (auto it = tokens.cbegin() + 1; it != tokens.cend(); ++it) {
                        m_out << (*it) << " ";
                    }
                    m_out << std::endl;
                } else {
                    m_out << "Unknown command " << command << ". Use /help for a list of commands." << std::endl;
                }
            }
        } else {
            // Otherwise, send the string as-is via a Say packet.
            if (checkCanSendSay()) {
                sendSay(str);
            }
        }
        // Successfully ran command
        return true;
    }

    bool checkCanSendSay() {
        auto clientState = getState();
        if (clientState == archipelago::ClientState::connected) {
            return true;
        } else {
            if (clientState == archipelago::ClientState::disconnected) {
                m_out << "Cannot send message to server while disconnected." << std::endl << "Use \"/connect\" to connect or \"/help\" for command help." << std::endl;
            }
            else {
                m_out << "Cannot send a message while still connecting, please wait." << std::endl;
            }
            return false;
        }
    }

    void commandConnect(const std::vector<std::string>& args) {
        // Should have 2-3 arguments (args array is UNIX-like in that the first token is the command)
        if (args.size() < 3 || args.size() > 4) {
            m_out << "Usage: /connect <server> <player> [password]";
            return;
        }
        const std::string& serverUrl = args.at(1);
        const std::string& playerName = args.at(2);
        const std::string* password = args.size() > 3 ? &(args.at(3)) : nullptr;
        m_out << "Connecting to " << serverUrl << " as " << playerName;
        if (password != nullptr) {
            m_out << " (with password)";
        }
        if (m_gameName.has_value()) {
            m_out << " (as game " << (*m_gameName) << ")" << std::endl;
        }
        m_out << "..." << std::endl;
        connect(serverUrl, playerName, password);
    }

    void commandUUID(const std::vector<std::string>& args) {
        bool create = args.size() >= 2 && args[1] == "create";
        try {
            auto uuid = archipelago::getCachedPlayerUUID();
            if (uuid.has_value()) {
                m_out << "Your client UUID is { " << *uuid << " }." << std::endl;
            } else {
                if (create) {
                    m_out << "No cached client UUID exists." << std::endl;
                    m_out << "Created a new UUID { " << archipelago::getPlayerUUID(true) << " }.";
                } else {
                    m_out << "Could not look up your client UUID. (You may not have an Archipelago-generated client ID cached yet.)" << std::endl;
                }
            }
        } catch (std::runtime_error e) {
            m_out << "Unable to look up your UUID: " << e.what() << std::endl;
        }
    }

    void commandCacheDir(const std::vector<std::string>& args) {
        try {
            m_out << "Archipelago cache directory: " << archipelago::getPlayerArchipelagoCacheDirectory() << std::endl;
        } catch (std::runtime_error e) {
            m_out << "Unable to look up the cache directory: " << e.what() << std::endl;
        }
    }

    void commandHelp(const std::vector<std::string>& args) {
        // TODO: Look at args and see if a command was given to get details on
        m_out << "Archipelago Terminal Client commands:" << std::endl;
        m_out << "-------------------------------------" << std::endl;
        m_out << std::endl;
        m_out << "/connect <server> <name> [password]" << std::endl;
        m_out << "    connect to <server> as player <name> (optionally using [password])" << std::endl;
        m_out << "/disconnect" << std::endl;
        m_out << "    disconnect from the Archipelago server" << std::endl;
        m_out << "/uuid" << std::endl;
        m_out << "    display your client UUID" << std::endl;
        m_out << "/help" << std::endl;
        m_out << "    display this help" << std::endl;
        m_out << "/say" << std::endl;
        m_out << "    say a message (after command parser tokenization)" << std::endl;
        m_out << "/game <game>" << std::endl;
        m_out << "    enable \"game\" mode" << std::endl;
        m_out << "/tracker" << std::endl;
        m_out << "    enable \"tracker\" mode (default)" << std::endl;
        m_out << "/quit, /exit" << std::endl;
        m_out << "    exit the client" << std::endl;
        m_out << std::endl;
        m_out << "Anything that is not a command (starts with a \"/\") will be sent to the server as-is." << std::endl;
    }

    void commandSay(const std::vector<std::string>& args) {
        if (!checkCanSendSay()) {
            return;
        }
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
        sendSay(message);
    }

    void commandStatus(const std::vector<std::string>& args) {
        m_out << "Websocket: ";
        switch (m_socket.getReadyState()) {
        case ix::ReadyState::Connecting:
            m_out << "connecting";
            break;
        case ix::ReadyState::Open:
            m_out << "open";
            break;
        case ix::ReadyState::Closing:
            m_out << "closing";
            break;
        case ix::ReadyState::Closed:
            m_out << "closed";
            break;
        }
        m_out << std::endl << "Client state: ";
        switch (getState()) {
        case archipelago::ClientState::disconnected:
            m_out << "disconnected";
            break;
        case archipelago::ClientState::connecting:
            m_out << "connecting (no response from server yet)";
            break;
        case archipelago::ClientState::websocketConnected:
            m_out << "websocket connected (waiting for Archipelago start)";
            break;
        case archipelago::ClientState::receivedRoomInfo:
            m_out << "received room info, response not sent";
            break;
        case archipelago::ClientState::sentGetDataPackage:
            m_out << "requested data package, waiting for data";
            break;
        case archipelago::ClientState::receivedDataPackage:
            m_out << "received data package, response not sent";
            break;
        case archipelago::ClientState::sentConnect:
            m_out << "sent connection request";
            break;
        case archipelago::ClientState::connected:
            m_out << "connected";
            break;
        case archipelago::ClientState::connectionRefused:
            m_out << "websocket connected, active connection refused";
            break;
        default:
            m_out << "internal error";
        }
        m_out << std::endl;
    }

    void commandGame(const std::vector<std::string>& args) {
        if (args.size() != 2) {
            m_out << "Usage: /game <name>" << std::endl;
            return;
        }
        if (getState() == archipelago::ClientState::disconnected) {
            m_gameName = args[1];
        } else {
            m_out << "Cannot set game mode after connecting, /disconnect first." << std::endl;
        }
    }

    void commandReleaseLocation(const std::vector<std::string>& args) {
        if (args.size() < 2) {
            m_out << "Usage: /release_location <location ID> ..." << std::endl;
            return;
        }
        if (getState() != archipelago::ClientState::connected) {
            m_out << "Can only release checks when connected." << std::endl;
            return;
        }
        // Since this is intended for debugging, if given a single ID, use sendLocationCheck(), and
        // if given multiple IDs, use sendLocationChecks().
        if (args.size() == 2) {
            // Parse the ID first
            int locationId = 0;
            try {
                locationId = std::stoi(args[1]);
            } catch (const std::invalid_argument& e) {
                m_out << "Invalid location ID \"" << args[1] << "\": location must be an integer." << std::endl;
                return;
            } catch (const std::out_of_range& e) {
                m_out << "Invalid location ID \"" << args[1] << "\": location ID is out of allowed range." << std::endl;
                return;
            }
            sendLocationCheck(locationId);
        } else {
            std::vector<archipelago::location_id_t> locations;
            for (auto it = args.cbegin() + 1; it != args.cend(); ++it) {
                const std::string& locationIdString = *it;
                try {
                    locations.push_back(std::stoi(locationIdString));
                } catch (const std::invalid_argument& e) {
                    m_out << "Invalid location ID \"" << locationIdString << "\": location must be an integer." << std::endl;
                    return;
                }
                catch (const std::out_of_range& e) {
                    m_out << "Invalid location ID \"" << locationIdString << "\": location ID is out of allowed range." << std::endl;
                    return;
                }
            }
            sendLocationChecks(locations);
        }
    }

    void commandTracker(const std::vector<std::string>& args) {
        if (args.size() != 1) {
            m_out << "Usage: /tracker" << std::endl;
            return;
        }
        if (getState() == archipelago::ClientState::disconnected) {
            m_gameName = std::nullopt;
        } else {
            m_out << "Cannot set tracker mode after connecting, /disconnect first." << std::endl;
        }
    }

    virtual void onPrintJSON(const archipelago::packets::PrintJSON& json) override {
        // This is the function that it's important to override: the ability to actually print text to the screen
        for (auto it = json.data.cbegin(); it != json.data.cend(); ++it) {
            auto& data = *it;
            if (data.text.has_value()) {
                // For now, write it raw
                m_out << *(data.text);
            }
        }
    }
};

int main(int argc, char* argv[])
{
    // Required under Windows
    ix::initNetSystem();

    std::cout << "Welcome to AP Client!" << std::endl;
    std::cout << "Use \"/connect <server> <player> [password]\" to connect or \"help\" to display additional commands." << std::endl;
    auto app = archipelago::TerminalClient(std::cin, std::cout);
    app.addCommand("uuid", showUUID, "display your client UUID", "[create]", "Display the cached Archipelago UUID if it exists.\nIf it does not exist and \"create\" is given, a new UUID will be generated.");
    app.addCommand("cachedir", showCacheDir);
    app.run();

    return 0;
}
