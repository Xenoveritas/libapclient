/*! \file apclient.cpp
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

#include <algorithm>
#include <stdexcept>

#include <ftxui/component/app.hpp>
#include <ftxui/component/component.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/color.hpp>

// Tell Windows not to break everything.
#define NOMINMAX
#include <ixwebsocket/IXNetSystem.h>

#include "commands.h"
#include "console_component.h"

// Forward declarations of commands:
void commandQuit(archipelago::SimpleClient& client, const std::vector<std::string>& args);
void commandStatus(archipelago::SimpleClient& client, const std::vector<std::string>& args);
void commandReceived(archipelago::SimpleClient& client, const std::vector<std::string>& args);
void commandMissing(archipelago::SimpleClient& client, const std::vector<std::string>& args);
void commandListItems(archipelago::SimpleClient& client, const std::vector<std::string>& args);
void commandListLocations(archipelago::SimpleClient& client, const std::vector<std::string>& args);

/*! \brief APClient implementation, using [FTXUI](https://github.com/ArthurSonzogni/FTXUI)
 * to generate the client UI.
 */
class APClient : public archipelago::SimpleClient {
private:
    // ftxui *really* wants this to be a shared pointer
    std::shared_ptr<ConsoleComponent> m_console;
    ftxui::Closure m_quitClosure;
    ftxui::App* m_app;
    std::string m_gameName;
    archipelago::GameData m_playerGameData;
    archipelago::LocationTracker<bool> m_locationTracker;
    archipelago::ItemTracker<> m_itemTracker;

public:
    APClient() : archipelago::SimpleClient(), m_console(new ConsoleComponent()), m_app(nullptr), m_locationTracker(), m_itemTracker() {
        // Add our commands
        addCommand("quit", commandQuit, "exits the client");
        addAlias("exit", "quit");
        addCommand("received", commandReceived, "lists items that have been received");
        addCommand("missing", commandMissing, "lists locations that have not been checked");
        addCommand("items", commandListItems, "list all items in the current game");
        addCommand("locations", commandListLocations, "list all locations in the current game");
        addCommand("say", command::say, {
            .help = "sends a give chat message",
            .arguments = "<message>...",
            .detailedHelp = "Sends a given chat message. This may not function quite the same way as might be expected: the message will go through the general tokenizer before being sent. This means \"/say 'a message' will have the quotes removed when sent, and extra spaces (like \"/say  a  message\") will be removed (producing \"a message\")."
        });
        addAlias("s", "say");
        addCommand("status", commandStatus, "shows current client status");
        addCommand("uuid", command::uuid, {
            .help = "display your client UUID",
            .arguments = "[create]",
            .detailedHelp = "Display the cached Archipelago UUID if it exists.\nIf it does not exist and \"create\" is given, a new UUID will be generated."
        });
        addCommand("cachedir", command::cacheDir);
        addCommand("get", command::get, "requests a value from the server");
    }

    virtual void createConnect(archipelago::packets::Connect& connect) override {
        // Take over this ourselves, mainly to do:
        connect.items_handling = archipelago::packets::ItemsHandling::all;
        connect.slot_data = true;
        connect.tags.push_back(archipelago::tag::kTextOnly);
    }

    virtual void write(const std::string& message, archipelago::MessageType type = archipelago::MessageType::basic) override {
        m_console->write(message, type);
    }

    /*! \brief Write a line, ending with a newline, to the client.
     *
     * \param message the message to write
     */
    virtual void writeLn(const std::string& message = std::string(), archipelago::MessageType type = archipelago::MessageType::basic) override {
        m_console->writeLn(message, type);
        // Also ask the app to re-render
        if (m_app != nullptr) {
            m_app->RequestAnimationFrame();
        }
    }

    virtual void onConnected(const archipelago::packets::Connected& connected) override {
        // Request the data package for the game our player is playing, if possible
        auto slotInfo = connected.slot_info.getNetworkSlotByName(m_playerName);
        if (slotInfo != nullptr) {
            write("Connected with player playing ", archipelago::MessageType::basic);
            writeLn(slotInfo->game, archipelago::MessageType::basic);
            m_gameName = slotInfo->game;
            sendGetDataPackage({ slotInfo->game });
        } else {
            m_gameName.clear();
            writeLn("Did not see player in player data", archipelago::MessageType::basic);
        }
        // Load up location data
        m_locationTracker << connected;
    }

    virtual void onReceivedItems(const archipelago::packets::ReceivedItems& receivedItems) override {
        std::lock_guard lock(m_mutex);
        // Store it
        m_itemTracker << receivedItems;
    }

    virtual void onDataPackage(const archipelago::packets::DataPackage& dataPackage) override {
        m_playerGameData.loadGame(dataPackage, m_gameName);
    }

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

    void writeReceived() {
        std::lock_guard lock(m_mutex);
        writeLn("The following items have been received:");
        if (m_itemTracker.items.empty()) {
            writeLn("   No items marked as received yet.");
        }
        for (auto item : m_itemTracker.items) {
            writeLn(std::format(" - {} (from {})", m_playerGameData.getItemName(item.item), item.player));
        }
    }

    void writeMissing() {
        std::lock_guard lock(m_mutex);
        writeLn("The following locations have not been checked:");
        auto locations = m_locationTracker.getLocationIds();
        if (locations.empty()) {
            writeLn("   No items marked as received yet.");
        } else {
            for (auto location : locations) {
                if (!m_locationTracker.getLocation(location)) {
                    writeLn(std::format(" - {}", m_playerGameData.getLocationName(location)));
                }
            }
        }
    }

    void writeItems() {
        std::lock_guard lock(m_mutex);
        writeLn("The following items are listed in the current game:");
        if (m_playerGameData.items.empty()) {
            writeLn("   No items found. (Game data not available?)");
        } else {
            auto items = m_playerGameData.getItemNames();
            std::sort(items.begin(), items.end());
            for (auto item : items) {
                writeLn(std::format(" - {}", item));
            }
        }
    }

    void writeLocations() {
        std::lock_guard lock(m_mutex);
        writeLn("The following locations are listed in the current game:");
        if (m_playerGameData.items.empty()) {
            writeLn("   No locations found. (Game data not available?)");
        }
        else {
            auto locations = m_playerGameData.getLocationNames();
            std::sort(locations.begin(), locations.end());
            for (auto location : locations) {
                writeLn(std::format(" - {}", location));
            }
        }
    }

    /*! \brief Run the client.
     */
    void run() {
        // Go ahead and write lines to the client
        writeLn("Welcome to AP Client!");
        writeLn("Use \"/connect <server> <player> [password]\" to connect or \"/help\" to display additional commands.", archipelago::MessageType::help);
        std::string command;
        auto input = ftxui::Input(&command) | ftxui::CatchEvent([&](ftxui::Event event) {
            if (event == ftxui::Event::Return) {
                // Process the current command
                handleCommand(command);
                command.clear();
                return true;
            }
            return false;
        });
        auto container = ftxui::Container::Vertical({});
        container->Add(m_console);
        container->Add(input);
        container->SetActiveChild(input);
        auto renderer = ftxui::Renderer(container, [&] {
            return container->Render();
        });
        auto screen = ftxui::App::Fullscreen();
        m_app = &screen;
        m_quitClosure = screen.ExitLoopClosure();
        LIBAPCLIENT_LOG("Starting AP client app");
        screen.Loop(renderer);
    }

    void stop() {
        if (m_quitClosure) {
            // Blank out this so that future calls to writeLn won't try and use it,
            // but it's otherwise handled via the loop closing.
            m_app = nullptr;
            m_quitClosure();
            m_quitClosure = nullptr;
        }
    }
};

void commandQuit(archipelago::SimpleClient& client, const std::vector<std::string>& args) {
    reinterpret_cast<APClient&>(client).stop();
}

void commandStatus(archipelago::SimpleClient& client, const std::vector<std::string>& args) {
    reinterpret_cast<APClient&>(client).writeStatus();
}

void commandReceived(archipelago::SimpleClient& client, const std::vector<std::string>& args) {
    reinterpret_cast<APClient&>(client).writeReceived();
}

void commandMissing(archipelago::SimpleClient& client, const std::vector<std::string>& args) {
    reinterpret_cast<APClient&>(client).writeMissing();
}

void commandListItems(archipelago::SimpleClient& client, const std::vector<std::string>& args) {
    reinterpret_cast<APClient&>(client).writeItems();
}

void commandListLocations(archipelago::SimpleClient& client, const std::vector<std::string>& args) {
    reinterpret_cast<APClient&>(client).writeLocations();
}

int main(int argc, char* argv[]) {
    // Required under Windows
    ix::initNetSystem();

    auto app = APClient();
    app.run();

    return 0;
}
