/*! \file libapclient.h
 * \brief Archipelago client written in C++.
 */

#ifndef LIBAPCLIENT_LIBAPCLIENT_H_
#define LIBAPCLIENT_LIBAPCLIENT_H_

#include <map>
#include <set>
#include <vector>
#include <deque>
#include <chrono>
#include <mutex>
#include <atomic>
#include <stdexcept>

#include <ixwebsocket/IXWebSocket.h>

#include "packets.h"
#include "utils.h"

namespace archipelago {

// Dump the tags into their own namespace. Since they're strings they can't be a class enum.

/// Tags that can be sent via Connect.
namespace tag {
    /// Tag indicating a client is a reference client.
    const auto kReferenceClient = "AP";
    /// %Client participates in the DeathLink mechanic, therefore will send and receive DeathLink bounce packets.
    const auto kDeathLink = "DeathLink";
    /// Indicates the client is a hint game, made to send hints instead of locations.
    const auto kHintGame = "HintGame";
    /// Indicates the client is a tracker, made to track instead of sending locations.
    const auto kTracker = "Tracker";
    /// Indicates the client is a basic client, made to chat instead of sending locations.
    const auto kTextOnly = "TextOnly";
    /// Indicates the client does not want to receive text messages, improving performance if not needed.
    const auto kNoText = "NoText";
};

/*!
 * \brief Client room information.
 *
 * There's a good chance this will (eventually) be changed to not extend
 * packets::RoomInfo and instead store information on its own.
 */
class ClientRoomInfo : public packets::RoomInfo {
public:
    /*!
     * Number of hint points the user has. This is updated via RoomUpdate
     * packets but is initially sent via a Connected packet.
     */
    int hint_points{ 0 };

    /// Team ID of the player.
    team_id_t team{ 0 };

    /// Team slot ID of the player.
    team_slot_id_t slot{ 0 };

    /*!
     * The time when the RoomInfo packet's time was last updated, on our side,
     * based on the local steady clock. When no packet has been received,
     * time_since_epoch() is 0.
     */
    std::chrono::time_point<std::chrono::steady_clock> m_localTime;

    /*!
     * Allows the room info to be updated based on an incoming
     * packets::RoomInfo packet. This will also update
     * ClientRoomInfo::m_localTime to the current system clock time.
     * \param roomInfo the packet to pull updated data from
     * \return itself
     */
    ClientRoomInfo& operator=(const packets::RoomInfo& roomInfo);

    /*!
     * Allows the room info to be updated based on an incoming
     * packets::Connected packet. This updates the player ID values and the
     * number of hint points the player has.
     * \param connected the packet to pull updated data from
     * \return itself
     */
    ClientRoomInfo& operator<<(const packets::Connected& connected);


    /*!
     * Allows the room info to be updated based on an incoming
     * packets::RoomUpdate packet. This updates the number of hint points the
     * player has.
     * \param roomUpdate the packet to pull updated data from
     * \return itself
     */
    ClientRoomInfo& operator<<(const packets::RoomUpdate& roomUpdate);

    /// Default constructor. Sets everything to blank or 0.
    ClientRoomInfo() : RoomInfo(), m_localTime() {}

    /*!
     * \brief Calculate the current time that the server likely thinks it is.
     *
     * This is based on std::chrono::steady_clock and the last time stamp given
     * by the server in a packets::RoomInfo packet. This can be used to set the
     * time in a packets::DeathLink. The result is only meaningful if a
     * packets::RoomInfo has been received from the server.
     *
     * \return a UNIX time stamp in seconds based on the observed amount of time
     *     that has passed since a given time was given by the server
     */
    double get_remote_time_now();
};

/*! \brief Indicates the state of the client.
 */
enum class ClientState {
    /*! \brief The client isn't connected at all (the initial state)
     */
    disconnected,

    /*! \brief Indicates that the client has opened the websocket but not
     * received anything from the server yet
     */
    connecting,

    /** \brief Indicatates that the websocket has successfully connected to the
     * server, but no information (specifcally, the packets::RoomInfo) has been
     * received yet.
     */
    websocketConnected,

    /*! \brief Indicates that the client has received the RoomInfo packet but
     * has not yet responded to it yet.
     *
     * This will be the state that Client::onRoomInfo(const packets::RoomInfo&)
     * sees the first time, as the client has received the package but not
     * replied to it.
     *
     * Only a single packets::GetDataPackage or a single packets::Connect should
     * be sent while in this state.
     */
    receivedRoomInfo,

    /*! \brief Indicates that the client has sent a request for a data package
     * (packets::GetDataPackage) but has yet to receive it.
     *
     * In theory, the client can send a additional packets::GetDataPackage
     * packets while in this state, or it can complete the initial connection
     * via client::sendConnect().
     */
    sentGetDataPackage,

    /*! \brief Indicates that the client sent a request for the data package and
     * has now received it, but has yet to reply with a packets::Connect.
     *
     * This state should only really exist for the duration of
     * Client::onDataPackage(const std::DataPackage&) - that method should call
     * Client::sendConnect() to send the packets::Connect request.
     *
     * In theory, the client can send a additional packets::GetDataPackage
     * packets while in this state, or it can complete the initial connection
     * via client::sendConnect().
     */
    receivedDataPackage,

    /*! \brief Indicates that the client has sent a packages::Connect to the
     * server, but has not received a reply.
     *
     * There are no valid packets to send during this state - the client should
     * wait for the connect to resolve either via
     * Client::onConnected(const packets::Connected&) or
     * Client::onConnectionRefused(const packets::ConnectionRefused&)
     */
    sentConnect,

    /*! \brief The handshake has been completed and the connection is now in a
     * state where gameplay messages can be sent and received.
     *
     * The majority of packets should only be sent in this state.
     */
    connected,

    /*! \brief The server rejected the client's connection request.
     *
     * In this state the connection is still "live" and may potentially be
     * recovered by sending another Connect packet which corrects the errors the
     * server reported (say, an invalid password).
     *
     * In theory the client may still send packets::GetDataPackage along with a
     * corrected packets::Connect while in this state.
     */
    connectionRefused
};

/*! \brief Exception raised when an attempt is made to do something while the
 * client is in a state where it can't do it.
 *
 * For example, calling
 * Client::connect(const std::string&, const std::string&, const std::string*)
 * while the client is disconnected.
 *
 * Note that this exception should be checked for and caught. Checking the state
 * prior to calling an API cannot guarantee that another thread won't change the
 * state out from under that call.
 */
class InvalidStateError : public std::runtime_error {
public:
    explicit InvalidStateError(const std::string& what_arg) : std::runtime_error(what_arg) {}
    explicit InvalidStateError(const char* what_arg) : std::runtime_error(what_arg) {}
};

/*! \brief The Archipelago client class.
 *
 * Handles a lot of the wire protocol for talking with the Archipelago server
 * and provides callbacks that can be used to handle incoming information from
 * the server.
 *
 * IXWebSocket creates a thread to handle web socket communication. Callbacks
 * will all happen in this thread, at least by default: IXWebSocket notifies
 * Client via an internal callback, which passes it to
 * receiveMessage(const std::string&) which by default invokes
 * parseMessage(const std::string&) to parse the message.
 *
 * Callbacks will always happen from within the web socket thread. It is
 * expected that most use cases will therefore have at least two threads: one
 * thread handling the game/user interface, and a second one that handles
 * receiving and sending network messages.
 *
 * Client#m_mutex is used to lock the critical areas of the client so that
 * public APIs may be called from any thread. This mutex will never be locked
 * when a callback is invoked and must therefore never be locked when calling
 * any public API.
 *
 * The basic client flow works like this:
 *
 * 1. The client object is created. It starts in the disconnected state.
 * 2. connect(const std::string& serverUrl, const std::string& playerName, const std::string* password)
 *    starts the connection process.
 * 3. The onRoomInfo(const packets::RoomInfo&) callback receives the
 *    basic room info.
 * 4. (Optional) The client sends a packets::GetDataPackage to request a data
 *    package from the server.
 * 5. (Optional) onDataPackage(const packets::DataPackage&) receives the data
 *    package.
 * 6. sendConnect() is used to start the connection. If a password is
 *    required, it should be set before this is called, otherwise
 *    promptForPassword() is invoked to request a password. This can
 *    either return the password directly (if possible) or wait for a password
 *    before setting Client::m_password and then invoking Client::sendConnect().
 * 7. Either onConnectionRefused(const packets::ConnectionRefused&) is
 *    called if the connection cannot be completed, or
 *    onConnected(const packets::Connected&) is called.
 */
class Client {
protected:
    /// The underlying web socket.
    ix::WebSocket m_socket;

    /*! \brief The current room info.
     *
     * This is only meaningful after the client has received the RoomInfo
     * packet.
     */
    ClientRoomInfo m_roomInfo{};

    /*! \brief The player name for the player.
     *
     * This defaults to a blank string but must be set via connect.
     */
    std::string m_playerName{ "" };

    /// \brief The password for this session, if necessary.
    std::optional<std::string> m_password{};

    /*! \brief The mutex used by the client.
     *
     * This mutex will never be locked when a callback is invoked. If a callback
     * locks it, it must unlock it before calling any public implementation
     * method. Protected implementation methods must never lock the mutex.
     */
    std::mutex m_mutex{};

private:
    /*! \brief Current client state.
     *
     * An atomic is used to ensure that writes are kept in order. On some
     * platforms, this means client state changes may be entirely lock-free.
     */
    std::atomic<ClientState> m_state{ ClientState::disconnected };

public:
    Client();
    ~Client();

    /*! \brief Start connecting to the server.
     *
     * The given player name and password will be stored for use in generating
     * the packets::Connect packet. This invokes IXWebSocket::start() which
     * will create a new thread to handle receiving messages from the server.
     * This is the thread that will process callbacks and invoke
     * receiveMessage(const std::string&). (Child classes can then dispatch
     * that to another thread if necessary.)
     *
     * \param serverUrl the URL for the server
     * \param playerName the name of the player
     * \param password an optional password, may be `nullptr` to not send a
     *        password
     * \throws InvalidStateError if the client is already connected
     */
    void connect(const std::string& serverUrl, const std::string& playerName, const std::string* password);

    /*! \brief Connect to the server without a password.
     *
     * See
     * connect(const std::string&, const std::string& playerName, const std::string*)
     * for details.
     * \param serverUrl the URL for the server
     * \param playerName the name of the player
     * \throws InvalidStateError if the client is already connected
     */
    void connect(const std::string& serverUrl, const std::string& playerName);

    /// Close the connection.
    void disconnect();

    /*! \brief Gets the current client state.
     *
     * This is thread-safe - the state is stored via a std::atomic. (However,
     * that means it can't be accessed in a `const` manner.)
     */
    ClientState getState();

    /*! \brief Callback called when the web socket connection to the server has
     * been established.The default implementation does nothing.
     */
    virtual void onWebSocketConnected() {}

    /*! \brief Indicates that the connection failed, providing additional
     * information on the failure.
     *
     * The default implementation logs the errors using
     * logError(const std::string&) and disconnect()s.
     *
     * \param errorInfo the error information
     */
    virtual void onWebSocketConnectionFailure(const ix::WebSocketErrorInfo& errorInfo);

    /*! \brief Called when the websocket closes.
     *
     * The client state will have been set to ClientState::disconnected.
     */
    virtual void onWebSocketDisconnect() {}

    /*! \brief Provides a packets::Connect packet to be filled out before it
     * gets sent to the server.
     *
     * The given packets::Connect packet will already have the player name and
     * password filled in if they were given to
     * connect(const std::string&, const std::string&, const std::string*).
     *
     * The default implementation only adds tag::kTextOnly to
     * packets::Connect::tags.
     */
    virtual void createConnect(packets::Connect& connect);

    /*! \brief Send the connect packet.
     *
     * This will create a connect packet and then call
     * createConnect(packet::Connect&) to allow additional client-specific
     * values to be filled out.
     *
     * If the room info indicates the room requires a password and m_password
     * is unset, promptForPassword() will be called.
     *
     * If either of those is still missing, this will return false and not send
     * a connect packet. Otherwise, this sends the connect packet via
     * sendPacket(const packets::Packet& packet).
     * \return `true` if the connection packet was sent, `false` if data is
     *         missing.
     */
    bool sendConnect();
    //void sendConnectUpdate(packets::ConnectUpdate& connectUpdate);

    /*!
     * \brief Send a sync message, requesting a complete ReceivedItems message
     * be sent back.
     *
     * The sync message has no payload.
     */
    void sendSync();

    /*! \brief Send a single location check.
     *
     * \param location_id the single location ID to send
     */
    void sendLocationCheck(location_id_t location_id);

    /*! \brief Send a set of location checks.
     *
     * \param location_ids the location IDs to send
     */
    void sendLocationChecks(const std::vector<location_id_t>& location_ids);

    //void sendLocationScouts(packets::LocationScouts& locationScouts);
    //void sendCreateHints(packets::CreateHints& createHints);

    void sendStatusUpdate(packets::ClientStatus clientStatus);

    /*! \brief Send a packets::Say packet, which is effectively text the player
     * is sending from a client chat interface.
     *
     * \param say the message to send
     */
    void sendSay(const std::string& say);

    /*! \brief Send a packets::Say packet, which is effectively text the player
     * is sending from a client chat interface.
     *
     * \param say the message to send
     */
    void sendSay(const char* say);

    /*! \brief Send a packets::GetDataPackage packet with the games list empty,
     * which requests data package for all games.
     *
     * The server will respond with a packets::DataPacket which will be
     * handled by onDataPacket(const packets::DataPacket&).
     */
    void sendGetDataPackage();

    /*! \brief Send a packets::GetDataPackage packet with the given games list.
     *
     * This asks the server to restrict the data returned to the given games.
     *
     * \param games the games to request data for
     */
    void sendGetDataPackage(const std::vector<std::string>& games);

    /// Sends a bounce packet to all other clients in the game.
    void sendBounce(const std::optional<std::vector<std::string>&> games,
        std::optional<std::vector<player_id_t>&> slots,
        std::optional<std::vector<std::string>&> tags,
        std::optional<json&> data
    );

    /*! \brief Request the server send a server-side value for a given key.
     *
     * This helper method solely generates a packets::Get. Extra JSON is allowed
     * with this packet within the protocol, which may be generated by
     * constructing a packets::Get directly and sending it via
     * sendPacket(const packets::Packet&).
     *
     * \param key the key to request from the server
     */
    void sendGet(std::string& key);

    /*! \brief Request the server send a server-side value for a given key.
     *
     * This helper method solely generates a packets::Get. Extra JSON is allowed
     * with this packet within the protocol, which may be generated by
     * constructing a packets::Get directly and sending it via
     * sendPacket(const packets::Packet&).
     *
     * \param key the key to request from the server
     */
    void sendGet(std::vector<std::string>& key);
    //void sendSet
    //void sendSetNotify(packets::SetNotify& setNotify);

    /*! \brief Send a single packet.
     *
     * This uses packets::Packet::to_json(json&) to create a JSON object that
     * can then be sent to sendMessage(const json&).
     *
     * \param packet the packet to send
     */
    void sendPacket(const packets::Packet& packet);

    /*! \brief Send a JSON message to the server.
     *
     * This converts the JSON to text via json::dumps().
     * \param payload the payload to send to the server
     * \throws InvalidStateError if the client is not connected to a server
     */
    void sendMessage(const json& payload);

    /*! \brief Called when a password is requested, but none has been provided.
     *
     * May return `std::nullopt` to indicate either the user canceled the
     * password request (or that no such attempt was made).
     *
     * The default implementation **always** returns `std::nullopt`.
     *
     * When called from the library, this will always be called from the web
     * socket's thread.
     *
     * \return either a player-provided string, or `std::nullopt`
     */
    virtual std::optional<std::string> promptForPassword() { return std::nullopt; }

    /*! \brief Called when a packets::RoomInfo packet is received.
     *
     * The packets::RoomInfo packet is the first packet received from the
     * server during the connection handshake. This callback is called after
     * the client has stored the information from the RoomInfo packet into its
     * local Client::m_roomInfo and has updated the state to
     * ClientState::roomInfoReceived.
     *
     * The default implementation calls sendConnect().
     *
     * If your client wants to get a game data packet, override this method and
     * onDataPackage(const packets::DataPackage&), calling sendGetDataPackage()
     * or sendGetDataPacket(const std::vector<std::string>&) here to request
     * data packages, and then call sendConnect() from
     * onDataPackage(const packets::DataPackage&).
     *
     * \param roomInfo the `RoomInfo` packet from the server
     */
    virtual void onRoomInfo(const packets::RoomInfo& roomInfo);

    /*! \brief Receive a Connected packet.
     *
     * This indicates that the connection was successful.
     * dispatchPacket(const std::string&, const json&) will have updated
     * the client state to be ClientState.connected before invoking this
     * callback.
     *
     * The default implementation does nothing.
     */
    virtual void onConnected(const packets::Connected& connected) {}

    /*! \brief Handler for when the connection is refused.
     *
     * The default logs the errors to logError() and disconnects the socket.
     * Clients may wish to keep the socket open as the error may be recoverable,
     * if, say, the password given was wrong. This requires a user interface
     * flow where the password can be re-requested from the user.
     *
     * \param connectionRefused the packet from the server explaining why the
     *        connection was refused
     */
    virtual void onConnectionRefused(const packets::ConnectionRefused& connectionRefused);

    /*! \brief Receive notification that the player has received items.
     *
     * The items received may be from another player or potentially the starting
     * inventory or even the local world, depending on the value of the
     * Connect::items_handling field given to the server during the connection.
     *
     * The default implementation does nothing.
     *
     * \param receivedItems the items received
     */
    virtual void onReceivedItems(const packets::ReceivedItems& receivedItems) {}

    /*! \brief Receive location information after requesting it via a
     * packets::LocationScout.
     *
     * The default implementation does nothing.
     */
    virtual void onLocationInfo(const packets::LocationInfo& locationInfo) {}

    /*! \brief Receive an update to the room info.
     *
     * See packets::RoomInfo for the current information that the server can
     * send to the client.
     *
     * The default implementation does nothing.
     */
    virtual void onRoomUpdate(const packets::RoomUpdate& roomUpdate) {}

    /*! \brief Receive text that's intended to be displayed to the user.
     *
     * The default implementation does nothing.
     * \param json the data describing the message to display to the user
     */
    virtual void onPrintJSON(const packets::PrintJSON& json) {}

    /*! \brief Receive a data package.
     *
     * Receives a data package that was requested via a
     * packets::GetDataPackage. The default implementation does nothing.
     */
    virtual void onDataPackage(const packets::DataPackage& dataPackage) {}

    /*! \brief Receive a bounced message from the server.
     *
     * See packets::Bounce from more details.
     *
     * The default implementation does nothing.
     */
    virtual void onBounced(const packets::Bounced& bounced) {}

    /*! \brief Receive notification that a packet was rejected by the server.
     *
     * As this is pretty much exclusively an implementation error, there isn't
     * much to be done besides logging the error. The default implementation
     * logs the error using logError(const std::string&).
     * \param invalidPacket information on the error the server encountered
     */
    virtual void onInvalidPacket(const packets::InvalidPacket& invalidPacket);

    /*! \brief Receive information received via a packets::Get request.
     *
     * The default implementation does nothing.
     */
    virtual void onRetrieved(const packets::Retrieved& retreived) {}

    /*! \brief Receive a reply to a packets::Set request.
     *
     * \param setReply the response
     */
    virtual void onSetReply(const packets::SetReply& setReply) {}

    /*! \brief Receives an incoming message from the server.
     *
     * The default implementation simply invokes
     * parseMessage(const std::string&).
     *
     * \param message the message received from the server, which should be
     *        raw JSON
     */
    virtual void receiveMessage(const std::string& message) { parseMessage(message); }

    /*! \brief Parse and dispatch an incoming message from the server.
     *
     * The message should be a JSON array of Packets.
     *
     * \param message the message received from the server, which should be
     *        raw JSON
     */
    void parseMessage(const std::string& message);

    /*! \brief Handle an incoming packet.
     *
     * This will be called once per packet in order each time a new packet
     * comes in. The default simply calls
     * dispatchPacket(const std::string&, const json&). Note that if this
     * method does not call dispatchPacket, the packet will effectively be
     * eaten: no callbacks will be called.
     *
     * \param command the value of the `cmd` field in the packet
     * \param packet the packet as a parsed JSON object
     */
    virtual void handlePacket(const std::string& command, const json& packet);

    /*! \brief Dispatch a packet to the appropriate event handlers.
     *
     * This will attempt to convert the raw JSON to a specific packet class.
     * If this fails, onJsonError(const json::exception&) will be invoked with
     * the JSON error.
     *
     * If the packet `cmd` isn't known, this passes it off to
     * receiveUnknownPacket(const json&).
     * \param command the command of the packet, this is used in place of the
     *     `cmd` field
     * \param packet the parsed JSON packet
     */
    void dispatchPacket(const std::string& command, const json& packet);

    /*! \brief Called when the client receives a message from the server that it
     * doesn't know how to handle, because the JSON object doesn't describe a
     * list of packets to handle.
     *
     * The default implementation logs an error (via
     * logError(const std::string&)) but otherwise ignores the message.
     *
     * \param rawMessage the raw message
     *\param message the parsed JSON, which is not a valid array of packets
     */
    virtual void receiveInvalidMessage(const std::string& rawMessage, const json& message);

    /*! \brief Receive a packet that the client doesn't know how to handle.
     *
     * The default implementation logs an error (via
     * logError(const std::string&)) but otherwise ignores the packet.
     *
     * \param unknown the unknown packet object
     */
    virtual void receiveUnknownPacket(const json& unknown);

    /*! \brief Called when an exception is thrown reading JSON from the server.
     *
     * The default simply logs the error via logError(const std::string&).
     * \param error the exception that was raised
     */
    virtual void onJsonError(const json::exception& error);

    /*! \brief Log an error message.
     *
     * User interfaces may wish to show the message to the user. The default
     * implementation writes to std::cerr.
     *
     * \param message the message to log
     */
    virtual void logError(const std::string& message);

    /*! \brief Gets the remote server time.
     *
     * This is only valid if called after a packets::RoomInfo packet has been
     * received.
     *
     * See ClientRoomInfo::get_remote_time_now() for details on how this is
     * determined.
     *
     * \return the remote server time as a UNIX timestamp in seconds
     */
    double get_remote_time_now();
};

/*! \brief
 * This extends the generic Client with some utilities intended to make writing
 * a tracker client easier. The tracker client maintains a list of items that
 * have been received and locations that have been checked.
 */
class TrackerClient : public Client {
public:
    TrackerClient();
    ~TrackerClient();

    /*! \brief Overrides the base Connected handler to grab the item
     * information.
     *
     * This simply locks the mutex and calls
     * updateCheckedLocations(const std::vector<location_id_t>& newLocations, bool)
     * with the checked locations from the Connected packet.
     *
     * \param connected the connected packet
     */
    virtual void onConnected(const packets::Connected& connected) override;

    /*! \brief Updates the received items list with the given items.
     *
     * \param receivedItems
     */
    virtual void onReceivedItems(const packets::ReceivedItems& receivedItems) override;

protected:
    /*! \brief Looks up the item count for a given item ID.
     * DOES NOT LOCK THE MUTEX.
     *
     * \param itemId the item ID to look up
     * \return the count of items for that item
     */
    unsigned int getItemCount(item_id_t itemId);

    /*!
     * Update received item information. DOES NOT LOCK THE MUTEX.
     *
     * \param newItems the newly received items
     * \param reset whether to clear the existing item information and reset it
     *        with this information. Defaults to `false`.
     */
    void updateReceivedItems(const std::vector<packets::NetworkItem>& newItems, bool reset = false);

    /*!
     * Update checked location information. DOES NOT LOCK THE MUTEX.
     *
     * \param newChecks the new check information
     * \param reset whether to clear the existing checked location information
     *        and reset it with this information
     */
    void updateCheckedLocations(const std::vector<location_id_t>& newLocations, bool reset = false);

    /*!
     * The item IDs. Note that the same item can appear more than once (for
     * example, ammo drops) and that order can matter.
     */
    std::vector<packets::NetworkItem> m_receivedItems;

    /// Received item IDs to the counts of times they've been received.
    std::map<item_id_t, unsigned int> m_receivedItemMap;

    /// Location IDs that have been marked as checked. This is updated by RoomUpdate packets.
    std::set<location_id_t> m_checkedLocationSet;
};

/*!
 * This is a TrackerClient with additional code designed to hold on to a deque
 * of location checks. Basically, the client will add newly discovered items to
 * the end of the deque, and it's expected that the game itself will pull in
 * items that it's handled from the start of the deque.
 */
class GameClient : public TrackerClient {
public:
    GameClient() : TrackerClient(), m_itemDeque() { }

    /*!
     * Receive items and add them to the deque of received items.
     *
     * \param receivedItems the received items packet
     */
    virtual void onReceivedItems(const packets::ReceivedItems& receivedItems) override;

    /*!
     * Pops a single item off the deque. Many games can only process receiving a
     * single item at a time and will need to wait before they can process the
     * next one anyway. As this locks the mutex, repeatedly calling popItem() in
     * the same thread is slow!
     *
     * \return the popped item or std::nullopt if the deque is empty
     */
    std::optional<packets::NetworkItem> popItem();

    /*!
     * Pops up to count items off the queue.
     *
     * \return the popped items
     */
    std::vector<packets::NetworkItem> popMultipleItems(size_t count = SIZE_MAX);
protected:
    /*!
     * A deque of NetworkItems to process. (The full NetworkItem is kept so that
     * the player that sent the item can be reported to the player.)
     */
    std::deque<packets::NetworkItem> m_itemDeque;

    /*!
     * The index of the last processed item. This index is important! It needs
     * to be saved in some fashion (preferably with the game's own save data) -
     * it's the index of the next item that should be given to the player of the
     * game.
     */
    size_t m_currentIndex{ 0 };

    /// Push items to the queue. DOES NOT LOCK THE MUTEX.
    void pushMultipleItems(const std::vector<packets::NetworkItem>& newItems);
};

}

#endif // LIBAPCLIENT_LIBAPCLIENT_H_