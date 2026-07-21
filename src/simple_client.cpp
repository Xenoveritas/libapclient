/*! \file simple_client.cpp */
#include "libapclient/simple_client.h"
#include "libapclient/tokenizer.h"

namespace archipelago {

void SimpleClient::addCommand(
    const std::string& name,
    const CommandFunction& command,
    const CommandOptions& options
) {
    // First part is simple:
    const bool success = m_commands.insert({name, command}).second;
    if (!success) {
        if (!options.replaceExisting) {
            throw std::runtime_error(std::format("There is already a command bound to {}", name));
        }
        // For now, if the element exists, remove it entirely:
        removeCommand(name);
        // And then add it as if it always succeeded
        if (!m_commands.insert({ name, command }).second) {
            throw std::runtime_error(std::format("Could not insert command {} even after removing it - possible threading issue?", name));
        }
    }
    if (!options.help.empty()) {
        // Add help
        std::shared_ptr<CommandHelpData> helpData = std::make_shared<CommandHelpData>(CommandHelpData{
            .name = name,
            .basicHelp = options.help,
            .arguments = options.arguments,
            .detailedHelp = options.detailedHelp
        });
        m_helpList.push_back(helpData);
        // Add the reference to this help to the m_helpData map
        m_helpData.insert({name, helpData});
    }
}

bool SimpleClient::removeCommand(const std::string& name) {
    // First, find the command
    auto iter = m_commands.find(name);
    if (iter == m_commands.end()) {
        return false;
    }
    m_commands.erase(iter);
    // Go through the m_helpList and remove it if it exists there
    for (auto listIter = m_helpList.begin(); listIter != m_helpList.end(); ++listIter) {
        if (listIter->get()->name == name) {
            m_helpList.erase(listIter);
            // The same name can't be added twice (or shouldn't be able to be) so break here
            break;
        }
    }
    // Also remove stored help data from the map
    auto helpIter = m_helpData.find(name);
    if (helpIter != m_helpData.end()) {
        m_helpData.erase(helpIter);
    }
    return true;
}

void SimpleClient::addAlias(const std::string& name, const std::string& originalCommand) {
    // Get the original command
    const CommandFunction* commandFunc = lookupCommand(originalCommand);
    if (commandFunc == nullptr) {
        throw std::runtime_error(std::format("Cannot alias {0} to {1} - nothing bound to {1}", name, originalCommand));
    }
    const bool success = m_commands.insert({name, *commandFunc}).second;
    if (!success) {
        throw std::runtime_error(std::format("Cannot alias {0} to {1} - command already exists for {0}", name, originalCommand));
    }
    auto helpIter = m_helpData.find(originalCommand);
    if (helpIter != m_helpData.end()) {
        // There may be no help data if the original command had no help data
        helpIter->second->addAlias(name);
    }
}

void SimpleClient::addDefaultCommands() {
    addCommand("help", commands::help, {
        .help = "show general help or help for the specified command",
        .arguments = "[command]",
        .detailedHelp = "provide help about how to use a command or detailed help about a specific command"
    });
    addCommand("connect", commands::connect, {
        .help = "connect to the server",
        .arguments = "<server> <name> [password]",
        .detailedHelp = "Connect to the server via the given WebSocket URL, player name, and optional password.\n\nIf a connection was already established, then /connect can be used with no arguments.\n\nIf the game requires a password and none was given, /password can be used to give it, and then /connect can be used to continue the connection."
    });
    addCommand("disconnect", commands::disconnect, "disconnect from the server");
    addCommand("ready", commands::ready, "mark self as ready on the server");
}

const CommandFunction* SimpleClient::lookupCommand(const std::string& name) const {
    auto iter = m_commands.find(name);
    if (iter == m_commands.end()) {
        return nullptr;
    } else {
        return &(iter->second);
    }
}

void SimpleClient::prompt(const std::string& prompt, const ReadStringCallbackFunction&& callback) {
    write(prompt);
    write(" ");
    registerReadStringCallback(std::move(callback));
}

void SimpleClient::registerReadStringCallback(const ReadStringCallbackFunction&& callback) {
    m_readStringCallback = callback;
}

void SimpleClient::receiveUserInput(const std::string& input) {
    if (m_readStringCallback) {
        m_readStringCallback(*this, input);
        // Unset the callback
        m_readStringCallback = nullptr;
    } else if (isCommand(input)) {
        dispatchCommand(input);
    } else {
        handleSay(input);
    }
}

void SimpleClient::dispatchCommand(const std::string& command) {
    if (command.empty()) {
        return;
    }
    std::vector<std::string> tokens;
    tokenize<std::string>(command, tokens);
    std::string command_name = tokens[0].substr(1);
    // Look up the command
    auto iter = m_commands.find(command_name);
    if (iter == m_commands.end()) {
        write("Unrecognized command ", MessageType::error);
        write(tokens[0], MessageType::error);
        writeLn(": use /help to list commands.", MessageType::error);
    } else {
        iter->second(*this, tokens);
    }
}

void SimpleClient::writeDetailedHelp(const std::string& name) {
    // Look up the detailed help for that command
    auto iter = m_helpData.find(name);
    if (iter == m_helpData.end()) {
        // no help data
        if (m_commands.find(name) == m_commands.end()) {
            writeLn(std::format("No command /{}", name), MessageType::error);
        } else {
            writeLn(std::format("No detailed help is available for /{}.", name), MessageType::help);
        }
    } else {
        writeLn(std::format("/{} {}", name, iter->second->arguments), MessageType::help);
        writeLn();
        if (iter->second->detailedHelp.empty()) {
            writeLn(iter->second->basicHelp, MessageType::help);
        } else {
            writeLn(iter->second->detailedHelp, MessageType::help);
        }
    }
}

void SimpleClient::writeHelp() {
    writeLn("The following commands are available:", MessageType::help);
    for (auto helpData : m_helpList) {
        write(std::format("/{}", helpData->name, helpData->arguments), MessageType::help);
        if (!helpData->arguments.empty()) {
            write(" ", MessageType::help);
            write(helpData->arguments, MessageType::help);
        }
        writeLn();
        writeLn(std::format("    {}", helpData->basicHelp), MessageType::help);
    }
}

void SimpleClient::writeUsageHelp(const std::string& name, const std::string& error) {
    const std::string& sanitizedName = name.size() > 1 && name[0] == '/' ? name.substr(1) : name;
    auto iter = m_helpData.find(sanitizedName);
    if (iter == m_helpData.end()) {
        // No help data. Assume the command exists and write out basic usage data.
        writeLn(std::format("Usage: /{}", sanitizedName), MessageType::help);
    } else {
        auto helpData = iter->second;
        write(std::format("Usage: /{}", sanitizedName), MessageType::help);
        if (!helpData->arguments.empty()) {
            write(" ", MessageType::help);
            write(helpData->arguments, MessageType::help);
        }
        writeLn();
    }
    if (!error.empty()) {
        writeLn(error, MessageType::error);
    }
}

TerminalClient::TerminalClient(
    std::istream& inStream,
    std::ostream& outStream
) : SimpleClient(), m_in(inStream), m_out(outStream), m_running(false), m_prompt(">") {
    addCommand("exit", [this](SimpleClient&, const std::vector<std::string>& arguments) {
        this->m_running = false;
    }, "exit the client");
    addAlias("quit", "exit");
}

void TerminalClient::write(const std::string& message, MessageType type) {
    m_out << message;
}

void TerminalClient::writeLn(const std::string& message, MessageType type) {
    m_out << message << std::endl;
}

void TerminalClient::prompt(const std::string& prompt, const ReadStringCallbackFunction&& callback) {
    m_prompt = prompt;
    SimpleClient::registerReadStringCallback(std::move(callback));
}

void TerminalClient::run() {
    std::string str;
    m_running = true;
    while(m_running) {
        m_out << m_prompt << " " << std::flush;
        if (!std::getline(m_in, str)) {
            break;
        }
        // After getting a line, reset the prompt
        m_prompt = ">";
        // Pass off to the command parser.
        SimpleClient::receiveUserInput(str);
    }
}

namespace commands {

void connect(SimpleClient& client, const std::vector<std::string>& arguments) {
    if (arguments.size() < 2 || arguments.size() > 4) {
        client.writeUsageHelp(arguments[0]);
        return;
    }
    auto url = arguments[1];
    if (arguments.size() < 3) {
        // No player argument. Request it.
        client.prompt("Player slot:", [url](SimpleClient& client, const std::string& input) {
            try {
                client.connect(url, input, nullptr);
            } catch (const InvalidStateError&) {
                client.write("Cannot connect: already connected.", MessageType::error);
            }
        });
        return;
    }
    auto player = arguments[2];
    const std::string* password = arguments.size() > 3 ? &(arguments[3]) : nullptr;
    try {
        client.connect(url, player, password);
    } catch (const InvalidStateError&) {
        client.write("Cannot connect: already connected.", MessageType::error);
    }
}

void disconnect(SimpleClient& client, const std::vector<std::string>& arguments) {
    if (arguments.size() > 1) {
        client.writeUsageHelp(arguments[0]);
        return;
    }
    try {
        client.disconnect();
        client.write("Disconnecting...", MessageType::basic);
        // Could just wait on the future but there are no threading guarantees right now
    } catch (const InvalidStateError&) {
        client.writeLn("Cannot disconnect: already disconnected.", MessageType::error);
    }
}

void help(SimpleClient& client, const std::vector<std::string>& arguments) {
    if (arguments.size() > 2) {
        client.writeLn("Usage: /help [command]", MessageType::help);
        client.writeLn("If given a command name, provides detailed help for that command.", MessageType::help);
        client.writeLn("With no command, lists all commands.", MessageType::help);
    } else if (arguments.size() == 2) {
        client.writeDetailedHelp(arguments[1]);
    } else {
        client.writeHelp();
    }
}

void ready(SimpleClient& client, const std::vector<std::string>& arguments) {
    if (arguments.size() > 1) {
        client.writeLn(std::format("Usage: /{}", arguments[0]));
    } else {
        // The actual Archipelago has this "toggle", sort of
        // The current state can be accessed by Getting _read_client_status_[team]_[slot]
        try {
            client.sendStatusUpdate(archipelago::packets::ClientStatus::ready);
        } catch (const InvalidStateError&) {
            client.writeLn("Cannot set ready state when not connected, /connect first.", MessageType::error);
        }
    }
}

}

}