/*! \file apclient.cpp
 * \brief Archipelago demo client
 *
 */

#include "libapclient/libapclient.h"
// Currently not part libapclient.h but may become part
#include "libapclient/simple_client.h"
#include "libapclient/tokenizer.h"
#include "libapclient/tracker.h"

#include <stdexcept>

#include <ftxui/component/app.hpp>
#include <ftxui/component/component.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/dom/elements.hpp>
#include "ftxui/screen/color.hpp"

// Tell Windows not to break everything.
#define NOMINMAX
#include <ixwebsocket/IXNetSystem.h>

#include "commands.h"

class ConsoleSpan {
private:
    std::string text;
    archipelago::MessageType type;
public:
    ConsoleSpan(const std::string& aText, archipelago::MessageType aType) : text(aText), type(aType) {}
    const std::string& getText() const { return text; }
    archipelago::MessageType getType() const { return type; }
    void appendText(const std::string& newText) {
        text.append(newText);
    }

    ftxui::Element Render() {
        auto element = ftxui::paragraph(text);
        switch (type) {
        case archipelago::MessageType::basic:
            break;
        case archipelago::MessageType::error:
            element |= ftxui::color(ftxui::Color::Red);
            break;
        case archipelago::MessageType::help:
            element |= ftxui::color(ftxui::Color::BlueLight);
            break;
        case archipelago::MessageType::server:
            break;
        }
        return element;
    }
};

class ConsoleLine {
private:
    std::vector<ConsoleSpan> spans;
public:
    ConsoleLine() : spans() {}
    // Copy constructor
    ConsoleLine(const ConsoleLine& other) = default;

    void appendText(const std::string& newText, archipelago::MessageType type) {
        if (spans.empty()) {
            // Just add a new span
            spans.push_back(std::move(ConsoleSpan(newText, type)));
        } else {
            // Check if the back span is the same type
            ConsoleSpan& existing = spans.back();
            if (existing.getType() == type) {
                existing.appendText(newText);
            } else {
                // Otherwise, make a new span
                spans.push_back(std::move(ConsoleSpan(newText, type)));
            }
        }
    }

    void clear() {
        spans.clear();
    }

    ftxui::Element Render() {
        if (spans.empty()) {
            return ftxui::text("");
        }
        if (spans.size() == 1) {
            return spans.front().Render();
        }
        // Otherwise, build a vbox
        ftxui::Elements elements;
        elements.reserve(spans.size());
        for (auto& span : spans) {
            elements.push_back(span.Render());
        }
        return ftxui::hflow(elements);
    }
};

// Forward declarations of commands:
void commandQuit(archipelago::SimpleClient& client, const std::vector<std::string>& args);
void commandStatus(archipelago::SimpleClient& client, const std::vector<std::string>& args);

/*! \brief APClient implementation, using [FTXUI](https://github.com/ArthurSonzogni/FTXUI)
 * to generate the client UI.
 */
class APClient : public archipelago::SimpleClient {
private:
    std::vector<ConsoleLine> m_console;
    ftxui::Closure m_quitClosure;
    ConsoleLine m_lastLine;
    ftxui::App* m_app;

    ftxui::Element RenderConsole() {
        ftxui::Elements lines;
        for (auto& line : m_console) {
            lines.push_back(line.Render());
        }
        return ftxui::flexbox(lines, {
            .direction = ftxui::FlexboxConfig::Direction::Column
        });
    }
public:
    APClient() : archipelago::SimpleClient(), m_console(), m_lastLine(), m_app(nullptr) {
        // Add our commands
        addCommand("quit", commandQuit, "exits the client");
        addCommand("say", command::say, {
            .help = "sends a give chat message",
            .arguments = "<message>...",
            .detailedHelp = "Sends a given cache message. This may not function quite the same way as might be expected: the message will go through the general tokenizer before being sent. This means \"/say 'a message' will have the quotes removed when sent, and extra spaces (like \"/say   a   message\") will be removed."
        });
        addCommand("status", commandStatus, "shows current client status");
        addCommand("uuid", command::uuid, {
            .help = "display your client UUID",
            .arguments = "[create]",
            .detailedHelp = "Display the cached Archipelago UUID if it exists.\nIf it does not exist and \"create\" is given, a new UUID will be generated."
        });
        addCommand("cachedir", command::cacheDir);
    }

    virtual void write(const std::string& message, archipelago::MessageType type = archipelago::MessageType::basic) override {
        m_lastLine.appendText(message, type);
    }

    /*! \brief Write a line, ending with a newline, to the client.
     *
     * \param message the message to write
     */
    virtual void writeLn(const std::string& message = std::string(), archipelago::MessageType type = archipelago::MessageType::basic) override {
        // Write the current message
        write(message, type);
        // And append our last line to the static lines
        m_console.push_back(m_lastLine);
        // And clear the last line
        m_lastLine.clear();
        if (m_app != nullptr) {
            m_app->RequestAnimationFrame();
        }
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
        auto renderer = ftxui::Renderer(input, [&] {
            return ftxui::vbox({
                RenderConsole()
                    | ftxui::vscroll_indicator
                    | ftxui::focusPositionRelative(0.0f, 1.0f)
                    | ftxui::yframe
                    | ftxui::focus
                    | ftxui::flex,
                input->Render()
            });
        });
        auto screen = ftxui::App::Fullscreen();
        m_app = &screen;
        m_quitClosure = screen.ExitLoopClosure();
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

int main(int argc, char* argv[]) {
    // Required under Windows
    ix::initNetSystem();

    auto app = APClient();
    app.run();

    return 0;
}
