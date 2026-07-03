#include "libapclient/simple_client.h"
#include "libapclient/tokenizer.h"

namespace archipelago {

void SimpleClient::addCommand(const std::string& name, const CommandFunction& command, const std::string& help, const std::string& detailedHelp) {
    // First part is simple:
    const bool success = m_commands.insert({name, command}).second;
    if (!success) {
        throw std::runtime_error(std::format("There is already a command bound to {}", name));
    }
    if (!help.empty()) {
        // Add help
        m_helpList.push_back(std::move(CommandHelpData(name, help, detailedHelp)));
        auto& insertedHelp = m_helpList.back();
        // Add the reference to this help to the m_helpData map
        m_helpData.insert({name, &insertedHelp});
    }
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
    addCommand("help", commands::help, "show help", "provide help about how to use a command or detailed help about a specific command");
    addCommand("connect", commands::connect, "connect to the server");
    addCommand("disconnect", commands::disconnect, "disconnect from the server");
}

const CommandFunction* SimpleClient::lookupCommand(const std::string& name) const {
    auto iter = m_commands.find(name);
    if (iter == m_commands.end()) {
        return nullptr;
    } else {
        return &(iter->second);
    }
}

void SimpleClient::handleCommand(const std::string& command) {
    if (command.empty()) {
        return;
    }
    // Only tokenize if this is a server command
    if (command[0] == '/') {
        std::vector<std::string> tokens;
        tokenize<std::string>(command, tokens);
        std::string command_name = tokens[0].substr(1);
        // Look up the command
        auto iter = m_commands.find(command_name);
        if (iter == m_commands.end()) {
            write("Unrecognized command ");
            write(tokens[0]);
            writeLn(": use /help to list commands.");
        } else {
            iter->second(*this, tokens);
        }
    } else {
        handleSay(command);
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
        writeLn(std::format("/{}", name), MessageType::help);
        if (iter->second->detailedHelp.empty()) {
            writeLn(iter->second->basicHelp, MessageType::help);
        } else {
            writeLn(iter->second->detailedHelp, MessageType::help);
        }
    }
}

void SimpleClient::writeHelp() {
    writeLn("The following commands are available:", MessageType::help);
    for (auto& helpData : m_helpList) {
        writeLn(std::format("/{}", helpData.name), MessageType::help);
        writeLn(std::format("    {}", helpData.basicHelp), MessageType::help);
    }
}

namespace commands {

void connect(SimpleClient& client, const std::vector<std::string>& arguments) {
    if (arguments.size() < 3 || arguments.size() > 4) {
        client.write("Usage: ", MessageType::error);
        client.write(arguments[0], MessageType::error);
        client.writeLn(" <server> <player>", MessageType::error);
    }
    auto url = arguments[1];
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
        client.write("Usage: ", MessageType::error);
        client.writeLn(arguments[0], MessageType::error);
    }
    try {
        client.disconnect();
    } catch (const InvalidStateError&) {
        client.write("Cannot disconnect: already disconnected.", MessageType::error);
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

}

}