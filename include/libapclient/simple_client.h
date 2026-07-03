#ifndef _LIBAPCLIENT_SIMPLE_CLIENT_H
#define _LIBAPCLIENT_SIMPLE_CLIENT_H

#include "libapclient.h"

#include <string>
#include <unordered_map>
#include <optional>
#include <iostream>
#include <functional>
#include <memory>

namespace archipelago {

class SimpleClient;

/*! \brief A function that implements a command.
 *
 * \param client the client that is running the command
 * \param arguments the arguments given to the command, including the
 *        argument used to start the command (so arguments[0] = "/command")
 */
typedef std::function<void(SimpleClient&, const std::vector<std::string>&)> CommandFunction;

enum class MessageType {
    basic,
    help,
    error,
    server
};

/*! \brief built-in commands */
namespace commands {

/*! \brief starts the client connection process
 */
void connect(SimpleClient& client, const std::vector<std::string>& arguments);
void disconnect(SimpleClient& client, const std::vector<std::string>& arguments);
void help(SimpleClient& client, const std::vector<std::string>& arguments);
void ready(SimpleClient& client, const std::vector<std::string>& arguments);

}

struct CommandOptions {
    /*! \brief whether to replace any existing command by that name or if an
     * exception should be thrown.
     */
    bool replaceExisting = false;
    /*! \brief help string, provided via just /help */
    std::string help = "";
    /*! \brief if given, help text describing arguments accepted */
    std::string arguments = "";
    /*! \brief detailed help to display via /help */
    std::string detailedHelp = "";
};

/*! \brief Metadata containing help for a command.
 */
class CommandHelpData {
public:
    std::string name;
    std::vector<std::string> aliases;
    std::string basicHelp;
    std::string arguments;
    std::string detailedHelp;
    CommandHelpData(
        const std::string& aName,
        const CommandOptions& options
    ) : name(aName), aliases(), basicHelp(options.help), arguments(options.arguments), detailedHelp(options.detailedHelp) {}

    void addAlias(const std::string& name) { aliases.push_back(name); }
};

/*! \brief Base class for a simple client that accepts user input.
 *
 * This does not handle any actual UI.
 */
class SimpleClient : public Client {
private:
    /// \brief A map of command names to the commands they run.
    std::unordered_map<std::string, CommandFunction> m_commands;
    /// \brief An ordered list of help data.
    std::vector<std::shared_ptr<CommandHelpData>> m_helpList;
    /// \brief A map of names to corresponding help metadata.
    std::unordered_map<std::string, std::shared_ptr<CommandHelpData>> m_helpData;
public:
    SimpleClient(bool bindDefaults = true) : Client() {
        if (bindDefaults) {
            addDefaultCommands();
        }
    }

    /*! \brief Adds the default commands to the client.
     *
     * The following commands are added:
     * 
     * | Name       | Implementation                                                                        |
     * |------------|---------------------------------------------------------------------------------------|
     * | help       | commands::help(SimpleClient& client, const std::vector<std::string>& arguments)       |
     * | connect    | commands::connect(SimpleClient& client, const std::vector<std::string>& arguments)    |
     * | disconnect | commands::disconnect(SimpleClient& client, const std::vector<std::string>& arguments) |
     * | ready      | commands::ready(SimpleClient& client, const std::vector<std::string>& arguments)      |
     */
    void addDefaultCommands();

    /*! \brief Add a client command with the associated options.
     *
     * See CommandOptions for what the options do.
     *
     * \param name the name of the command (without the starting "/")
     * \param command the command to run
     * \param options the options to use for the command
     * \throws std::runtime_error if there is already a command bound to `name`
     */
    void addCommand(
        const std::string& name,
        const CommandFunction& command,
        const CommandOptions& options
    );

    /*! \brief Add a client command with default options.
     *
     * \param name the name of the command (without the starting "/")
     * \param command the command to run
     * \throws std::runtime_error if there is already a command bound to `name`
     */
    void addCommand(
        const std::string& name,
        const CommandFunction& command
    ) {
        CommandOptions options;
        addCommand(name, command, options);
    }

    /*! \brief Add a client command with the associated help strings.
     *
     * An empty string is treated as "no help" and will prevent the command from
     * being shown in help lists. If the basic help string is empty, then the
     * detailed help string is ignored.
     *
     * \param name the name of the command (without the starting "/")
     * \param command the command to run
     * \param help the help string for the command
     * \throws std::runtime_error if there is already a command bound to `name`
     */
    void addCommand(
        const std::string& name,
        const CommandFunction& command,
        const std::string& help
    ) {
        addCommand(name, command, { .help = help });
    }

    /*! \brief Remove a command.
     *
     * \param name the command to remove
     * \return whether or not a command was removed - if the command didn't
     *         exist, this returns `false`
     */
    bool removeCommand(const std::string& name);

    /*! \brief Add an alias for an existing command.
     *
     * It's also possible to just add the same function under different names,
     * but that won't set up the help data to indicating aliases.
     *
     * \param name the alias name
     * \param originalCommand the name of the command to invoke with the alias
     * \throws std::runtime_error if the originalCommand doesn't exist
     */
    void addAlias(const std::string& name, const std::string& originalCommand);

    /** \brief Look up a command function.
     *
     * \param name the name of the command to look up
     * \return the command function for that name or nullptr if there is no
     *         command by that name
     */
    const CommandFunction* lookupCommand(const std::string& name) const;

    /*! \brief Generic write.
     *
     * \param message the text to write to the client terminal
     */
    virtual void write(const std::string& message, MessageType type = MessageType::basic) = 0;

    /*! \brief Write a line, ending with a newline, to the client.
     *
     * \param message the message to write
     */
    virtual void writeLn(const std::string& message = std::string(), MessageType type = MessageType::basic) = 0;

    /*! \brief override to forward a message from the client to
     * write(const std::string& message, MessageType type = MessageType::basic)
     */
    virtual void onPrintJSON(const archipelago::packets::PrintJSON& json) override {
        // This is the function that it's important to override: the ability to actually print text to the screen
        for (auto it = json.data.cbegin(); it != json.data.cend(); ++it) {
            auto& data = *it;
            if (data.text.has_value()) {
                // For now, write it raw
                write(*(data.text));
            }
        }
        writeLn();
    }

    /*! \brief Write out usage help for the given command.
     *
     * This is frequently used when the command is called incorrectly.
     *
     * \param name the name of the command to write help text for
     * \param error if given, an error message to show after the usage text
     */
    virtual void writeUsageHelp(const std::string& name, const std::string& error = std::string());

    /*! \brief Write out detailed help for the given command.
     */
    virtual void writeDetailedHelp(const std::string& name);

    /*! \brief Write out detailed help for the given command.
     */
    virtual void writeHelp();

    /*! \brief Check if the given string should be considered a command.
     *
     * The default implementation considers any string that starts with a '/' to
     * be a command.
     */
    virtual bool isCommand(const std::string& command) const {
        return command.size() >= 1 && command[0] == '/';
    }
    
    /*! \brief Sanitize a command name, removing whatever command prefix from
     * the name.
     *
     * The default implementation chbcks if the first character is a '/' and
     * removes it if it is.
     * 
     * This is used to santize command names given to any of the command
     * lookup functions.
     */
    virtual std::string_view sanitizeCommand(const std::string& command) {
        std::string_view view = command;
        if (command.size() > 0 && command[0] == '/') {
            view.remove_prefix(1);
        }
        return view;
    }

    /*! \brief handle a comment
     *
     * Parses an incoming command, dispatching it as appropriate.
     * \param command the command to parse
     */
    void handleCommand(const std::string& command);

    /*! \brief handle a message that doesn't trigger a client command.
     *
     * The default sends a say packet, assuming the client is connected, and
     * otherwise prints an error message.
     *
     * \param message the message the user wrote
     */
    virtual void handleSay(const std::string& message) {
        try {
            sendSay(message);
        } catch (const InvalidStateError&) {
            // This has a slight chance of generating the wrong message in weird circumstances.
            if (isDisconnected()) {
                writeLn("Cannot send a message to the server while disconnected.");
            } else {
                writeLn("Cannot send a message to the server until fully connected to the server.");
            }
        }
    }

    /*! \brief checks if the client is in the connected state
     *
     * \return true if the client is in the connected state
     */
    bool isFullyConnected() { return getState() == ClientState::connected; }

    /*! \brief checks if the client is in the connected state
     *
     * \return true if the client is in the connected state
     */
    bool isDisconnected() { return getState() == ClientState::disconnected; }
};

/*! \brief A simple client that reads and writes from file streams.
 */
class TerminalClient : public SimpleClient {
private:
    std::istream& m_in;
    std::ostream& m_out;
    bool m_running;
public:
    TerminalClient(
        std::istream& inStream,
        std::ostream& outStream
    ) : SimpleClient(), m_in(inStream), m_out(outStream), m_running(false) {
        addCommand("exit", [this](SimpleClient&, const std::vector<std::string>& arguments) {
            this->m_running = false;
        }, "exit the client");
        addAlias("quit", "exit");
    }

    /*! \brief Generic write.
     *
     * \param message the text to write to the client terminal
     */
    virtual void write(const std::string& message, MessageType type = MessageType::basic) override {
        m_out << message;
    }

    /*! \brief Write a line, ending with a newline, to the client.
     *
     * \param message the message to write
     */
    virtual void writeLn(const std::string& message, MessageType type = MessageType::basic) override {
        m_out << message << std::endl;
    }

    /*! \brief Run the client, reading from the input stream and passing
     * commands off to handleCommand.
     */
    void run() {
        std::string str;
        m_running = true;
        while(m_running) {
            m_out << "> " << std::flush;
            if (!std::getline(m_in, str)) {
                break;
            }
            // Pass off to the command parser.
            SimpleClient::handleCommand(str);
        }
    }
};

} // namespace archipelago

#endif // _LIBAPCLIENT_SIMPLE_CLIENT_H