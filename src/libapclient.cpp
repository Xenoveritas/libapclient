/*! \file libapclient.cpp
 * \brief main client implementation
 */

#include "libapclient/libapclient.h"
#include "libapclient/logger.h"

#include <ratio>
#include <chrono>
#include <iostream>

namespace archipelago {

ClientRoomInfo& ClientRoomInfo::operator=(const RoomInfo& roomInfo) {
    // basically, just copy everything over
    version = roomInfo.version;
    // the version of Archipelago which generated the multiworld.
    generator_version = roomInfo.generator_version;
    tags = roomInfo.tags;
    password = roomInfo.password;
    permissions = roomInfo.permissions;
    hint_cost = roomInfo.hint_cost;
    location_check_points = roomInfo.location_check_points;
    games = roomInfo.games;
    // TODO: Record if these changed
    datapackage_checksums = roomInfo.datapackage_checksums;
    seed_name = roomInfo.seed_name;
    time = roomInfo.time;
    // And update our time
    m_localTime = std::chrono::steady_clock::now();
    return *this;
}

ClientRoomInfo& ClientRoomInfo::operator<<(const packets::RoomUpdate& roomUpdate) {
    if (roomUpdate.hint_points.has_value()) {
        hint_points = *roomUpdate.hint_points;
    }
    if (roomUpdate.location_check_points.has_value()) {
        location_check_points = *roomUpdate.location_check_points;
    }
    if (roomUpdate.permissions.has_value()) {
        permissions = *roomUpdate.permissions;
    }
    return *this;
}

ClientRoomInfo& ClientRoomInfo::operator<<(const packets::Connected& connected) {
    hint_points = connected.hint_points;
    team = connected.team;
    slot = connected.slot;
    return *this;
}

double ClientRoomInfo::get_remote_time_now() {
    // Get the number of ticks between now and our previous time
    auto ticks = std::chrono::steady_clock::now() - m_localTime;
    // And convert that into a double number of seconds. The chrono library will do that for us.
    // Add to our starting time and we have our result.
    return time + ((std::chrono::duration<double>) ticks).count();
}

Client::Client() {
}
Client::~Client() {
    if (m_socket != nullptr) {
        delete m_socket;
        m_socket = nullptr;
    }
    // The web socket will stop automatically on deconstruction, so just let that happen on its own
}

void Client::connect(const std::string& serverUrl, const std::string& playerName, const std::string* password) {
    // Grab the lock for this
    std::lock_guard lock(m_mutex);
    if (m_state != ClientState::disconnected || m_socket != nullptr) {
        throw InvalidStateError("client is already connected");
    }
    // Create the socket
    m_socket = new ix::WebSocket();
    // Register the callback (as m_socket is part of this class, capturing this is fine -
    // m_socket will always exist for very slightly less time than this will)
    m_socket->setOnMessageCallback([this](const ix::WebSocketMessagePtr& message) {
        if (message->type == ix::WebSocketMessageType::Message) {
            // Pass this off to be parsed
            this->receiveMessage(message->str);
        }
        else if (message->type == ix::WebSocketMessageType::Open) {
            this->m_state = ClientState::websocketConnected;
            this->onWebSocketConnected();
        }
        else if (message->type == ix::WebSocketMessageType::Error) {
            this->onWebSocketConnectionFailure(message->errorInfo);
        }
        else if (message->type == ix::WebSocketMessageType::Close) {
            // Mark ourselves as disconnected
            this->m_state = ClientState::disconnected;
            this->onWebSocketDisconnect();
        }
        });
    // Archipelago wants per message deflate enabled and will currently generate a warning if it isn't
    m_socket->enablePerMessageDeflate();
    // Do not automatically attempt to reconnect - this likely requires additional
    // protocol support to properly redo the client auth
    m_socket->disableAutomaticReconnection();
    m_playerName = playerName;
    if (password != nullptr) {
        m_password = *password;
    }
    // The server provided by the Archipelago web host looks like <server>:<port>.
    // Also accept ws://<server>:<port> and wss://<server>:<port>.
    // For now, just make sure it starts with ws:// or wss://
    if (serverUrl.starts_with("ws://") || serverUrl.starts_with("wss://")) {
        // Use as-is
        m_socket->setUrl(serverUrl);
    } else {
        // In this case, just add "wss://" to it
        m_socket->setUrl(std::string("wss://").append(serverUrl));
    }
    // At this point, start up the socket
    LIBAPCLIENT_LOG("Starting up socket {}", m_socket->getUrl());
    m_socket->start();
    m_state = ClientState::connecting;
}

void Client::connect(const std::string& serverUrl, const std::string& playerName) {
    connect(serverUrl, playerName, nullptr);
}

void Client::disconnect_socket_run(std::promise<void> disconnect_promise) {
    LIBAPCLIENT_LOG("Disconnecting...");
    m_socket->stop();
    std::lock_guard lock(m_mutex);
    m_state = ClientState::disconnected;
    delete m_socket;
    m_socket = nullptr;
    LIBAPCLIENT_LOG("Socket closed.");
    disconnect_promise.set_value();
}

std::future<void> Client::disconnect() {
    std::lock_guard lock(m_mutex);
    if (m_state == ClientState::disconnected) {
        throw InvalidStateError("Client already disconnected");
    }
    // So here's where things get... hairy.
    // m_socket->stop() attempts to join its thread, and there's a chance this can
    // get called on the same thread (for example, the server indicates that the
    // client is incompatible, meaning there's no point in staying connected).
    //
    // thread.join() will throw a std::runtime_error when it detects that deadlock
    // condition (as a thread can't wait on itself).
    //
    // ix::~WebSocket() calls stop().
    //
    // Soooo... fork a thread whose sole job is to call stop(), wait for it to
    // return, grab the mutex, then delete the socket.
    m_state = ClientState::disconnecting;
    std::promise<void> disconnect_promise;
    std::future<void> disconnect_future = disconnect_promise.get_future();
    std::thread(&Client::disconnect_socket_run, this, std::move(disconnect_promise)).detach();
    return disconnect_future;
}

ClientState Client::getState() {
    // Just to make sure this is an atomic get, really, lock the mutex
    std::lock_guard lock(m_mutex);
    return m_state;
}

void Client::onWebSocketConnectionFailure(const ix::WebSocketErrorInfo& errorInfo) {
    logError(std::format("Connection error: {}", errorInfo.reason));
    // For now, always disconnect (prevent infinitely attempting to connect to a thing that will never answer)
    disconnect();
}

void Client::createConnect(packets::Connect& connect) {
    connect.tags.push_back(tag::kTextOnly);
}

// Send functions

bool Client::sendConnect() {
    packets::Connect connect;
    // Grab the lock while setting values from this
    std::unique_lock lock(m_mutex);
    connect.name = m_playerName;
    if (m_password.has_value()) {
        connect.password = *m_password;
    }
    // Drop the lock while trying to get the player UUID and while createConnect
    // is running. (It MUST be dropped for createConnect due to the contract that
    // all public APIs must never be called while the mutex is held.)
    lock.unlock();
    // Try and get the UUID
    connect.uuid = getPlayerUUID(true);
    createConnect(connect);
    // Re-grab the lock here.
    lock.lock();
    if (m_roomInfo.password && !connect.password.has_value()) {
        // See if the user wants to give us a password
        // MUST unlock for this - promptForPassword can potentially block
        // and then attempt to resurrect this on another thread.
        lock.unlock();
        auto password = promptForPassword();
        // FIXME: Check to see if the state has changed due to weird
        // interactions with promptForPassword
        lock.lock();
        if (password.has_value()) {
            m_password = *password;
            connect.password = *password;
        } else {
            // Give up.
            return false;
        }
    }
    sendUnlockedPacket(connect);
    m_state = ClientState::sentConnect;
    return true;
}
//void sendConnectUpdate(packets::ConnectUpdate& connectUpdate);

// Send a Sync message (which is empty).
void Client::sendSync() {
    auto j = json::array({ json::object({ { "cmd", packets::kPacketSync } }) });
    sendMessage(j);
}

void Client::sendLocationCheck(location_id_t location_id) {
    auto locations = std::vector<location_id_t>({ location_id });
    sendPacket(packets::LocationChecks(locations));
}

void Client::sendLocationChecks(const std::vector<location_id_t>& location_ids) {
    sendPacket(packets::LocationChecks(location_ids));
}

void Client::sendSay(const std::string& say) {
    sendPacket(packets::Say(say));
}

void Client::sendSay(const char* say) {
    sendPacket(packets::Say(say));
}

//void sendLocationScouts(packets::LocationScouts& locationScouts);
//void sendCreateHints(packets::CreateHints& createHints);
void Client::sendStatusUpdate(packets::ClientStatus clientStatus) {
    sendPacket(packets::StatusUpdate(clientStatus));
}

void Client::sendGetDataPackage() {
    sendPacket(packets::GetDataPackage());
}

void Client::sendGetDataPackage(const std::vector<std::string>& games) {
    sendPacket(packets::GetDataPackage(games));
}

//void sendGetDataPackage(packets::GetDataPackage& dataPackage);
//void sendBounce(packets::Bounce& bounce);
//void sendGet(packets::Get& get);
//void sendSet(packets::Set& set);
//void sendSetNotify(packets::SetNotify& setNotify);

void Client::sendPacket(const packets::Packet& packet) {
    // Basically, convert to JSON and then send.
    json j;
    packet.to_json(j);
    // Send the message within a list
    sendMessage(json::array({ j }));
}

void Client::sendUnlockedPacket(const packets::Packet& packet) {
    json j;
    packet.to_json(j);
    j = json::array({ j });
    const std::string text = j.dump();
    LIBAPCLIENT_LOG("Sending: {}", text);
    m_socket->sendUtf8Text(text);
}

void Client::sendMessage(const json& payload) {
    std::lock_guard lock(m_mutex);
    if (m_socket == nullptr) {
        throw InvalidStateError("Can't send message when disconnected");
    }
    const std::string text = payload.dump();
    LIBAPCLIENT_LOG("Sending: {}", text);
    m_socket->sendUtf8Text(text);
}

void Client::onRoomInfo(const packets::RoomInfo& roomInfo) {
    // If the room info packet is expected as part of the handshake, use it
    if (m_state == ClientState::receivedRoomInfo) {
        // If we're here... connect!
        sendConnect();
    }
}

void Client::onConnectionRefused(const packets::ConnectionRefused& refused) {
    logError("Server rejected connection:");
    for (auto& error : refused.errors) {
        logError(error);
    }
    disconnect();
}

void Client::onInvalidPacket(const packets::InvalidPacket& invalidPacket) {
    if (invalidPacket.original_cmd.has_value()) {
        logError(std::format("Server rejected a {} packet (bad {}): {}", *(invalidPacket.original_cmd), invalidPacket.type, invalidPacket.text));
    } else {
        logError(std::format("Server rejected a packet it failed to parse (bad {}): {}", invalidPacket.type, invalidPacket.text));
    }
}

void Client::parseMessage(const std::string& message) {
    // For debugging:
    LIBAPCLIENT_LOG("Received: {}", message);
    // Try and parse this as JSON
    json payload;
    try {
        payload = json::parse(message);
    } catch (const json::exception& error) {
        onJsonError(error);
        return;
    }
    // Payload should be a JSON array of Archipelago packets.
    if (payload.is_array()) {
        // Handle the array.
        for (auto it = payload.begin(); it != payload.end(); ++it) {
            const json& packet = *it;
            if (packet.is_object()) {
                // Make sure we know how to handle this
                auto cmdIt = packet.find("cmd");
                if (cmdIt != packet.end()) {
                    // Have a cmd field, make sure it's a string and then try and parse things
                    const json& cmdJson = *cmdIt;
                    if (cmdJson.is_string()) {
                        // Attempt to parse the string into a packet
                        const std::string& cmd = cmdJson;
                        try {
                            handlePacket(cmd, packet);
                        } catch (const json::exception& error) {
                            // Something went wrong with the JSON, either something is missing or
                            // it's malformed. In any case, just fall through and allow the
                            // unknown packet to be handled by receiveUnknownPacket.
                            onJsonError(error);
                        }
                        continue;
                    }
                }
            }
            // If here, packet wasn't handled
            receiveUnknownPacket(packet);
        }
    } else {
        receiveInvalidMessage(message, payload);
    }
}

void Client::handlePacket(const std::string& command, const json& packet) {
    dispatchPacket(command, packet);
}

void Client::dispatchPacket(const std::string& command, const json& packet) {
    // Handle state transitions here.
    // NOTE: mutex must NOT be locked when a callback is invoked!
    if (command == packets::kPacketReceivedItems) {
        onReceivedItems((const packets::ReceivedItems)packet);
    } else if (command == packets::kPacketPrintJSON) {
        onPrintJSON((const packets::PrintJSON)packet);
    } else if (command == packets::kPacketLocationInfo) {
        onLocationInfo((const packets::LocationInfo)packet);
    } else if (command == packets::kPacketRoomUpdate) {
        onRoomUpdate((const packets::RoomUpdate)packet);
    } else if (command == packets::kPacketBounced) {
        onBounced((const packets::Bounced)packet);
    } else if (command == packets::kPacketRetrieved) {
        onRetrieved((const packets::Retrieved)packet);
    } else if (command == packets::kPacketSetReply) {
        onSetReply((const packets::SetReply)packet);
    } else if (command == packets::kPacketDataPackage) {
        onDataPackage((const packets::DataPackage)packet);
    } else if (command == packets::kPacketRoomInfo) {
        const auto roomInfo = (const packets::RoomInfo)packet;
        {
            // Grab the lock while updating
            std::lock_guard lock(m_mutex);
            if (m_state == ClientState::websocketConnected) {
                m_state = ClientState::receivedRoomInfo;
                m_roomInfo = roomInfo;
            }
        }
        onRoomInfo(roomInfo);
    } else if (command == packets::kPacketConnected) {
        const auto connected = (const packets::Connected)packet;
        {
            // Grab the lock while updating
            std::lock_guard lock(m_mutex);
            m_state = ClientState::connected;
            // Update our room info
            m_roomInfo << connected;
        }
        onConnected((const packets::Connected)connected);
    } else if (command == packets::kPacketConnectionRefused) {
        m_state = ClientState::connectionRefused;
        onConnectionRefused((const packets::ConnectionRefused)packet);
    } else if (command == packets::kPacketInvalidPacket) {
        onInvalidPacket((const packets::InvalidPacket)packet);
    } else if (command == packets::kPacketDataPackage) {
        m_state = ClientState::receivedDataPackage;
        onDataPackage((const packets::DataPackage)packet);
    } else {
        receiveUnknownPacket(packet);
    }
}

void Client::receiveInvalidMessage(const std::string& rawMessage, const json& message) {
    logError("Invalid message from server");
    disconnect();
}

// Receive notification of an unknown packet. By default, just log.
void Client::receiveUnknownPacket(const json& unknown) {
    if (unknown.is_object()) {
        auto cmdIt = unknown.find("cmd");
        if (cmdIt == unknown.end()) {
            logError("Unable to handle packet: cmd is missing");
        } else {
            const json& cmd = *cmdIt;
            if (cmd.is_string()) {
                logError(std::format("Unable to handle packet {} (unknown/unimplemented packet)", (std::string)cmd));
            } else {
                logError(std::format("Unable to handle packet: cmd is {}, not a string", cmd.type_name()));
            }
        }
    } else {
        logError(std::format("Unable to handle packet: expected JSON object, was {}", unknown.type_name()));
    }
}

void Client::onJsonError(const json::exception& error) {
    logError(std::format("JSON error: {}", error.what()));
}

void Client::logError(const std::string& message) {
    LIBAPCLIENT_LOG("{}", message);
    // The above will double-log on non-Windows platforms, unless we exclude
    // the following:
#if !defined(LIBAPCLIENT_ENABLE_LOGGING) || defined(WIN32)
    std::cerr << message << std::endl;
#endif
}

double Client::get_remote_time_now() {
    // Log if an attempt is made to get the time before ready.
    if (m_state < ClientState::receivedRoomInfo) {
        logError("Attempt to get remote time before receiving RoomInfo - the result will be wrong and meaningless!");
    }
    return m_roomInfo.get_remote_time_now();
}

TrackerClient::TrackerClient() : Client(), m_receivedItems(), m_receivedItemMap(), m_checkedLocationSet() {
}
TrackerClient::~TrackerClient() {}

void TrackerClient::onConnected(const packets::Connected& connected) {
    std::lock_guard lock(m_mutex);
    // Update our received item set.
    m_checkedLocationSet.insert(connected.checked_locations.cbegin(), connected.checked_locations.cend());
}

void TrackerClient::onReceivedItems(const packets::ReceivedItems& receivedItems) {
    std::lock_guard lock(m_mutex);
    updateReceivedItems(receivedItems.items, false);
}

unsigned int TrackerClient::getItemCount(item_id_t type) {
    auto it = m_receivedItemMap.find(type);
    if (it == m_receivedItemMap.end()) {
        return 0;
    } else {
        return it->first;
    }
}

void TrackerClient::updateReceivedItems(const std::vector<packets::NetworkItem>& newItems, bool reset) {
    if (reset) {
        m_receivedItems.clear();
        m_receivedItemMap.clear();
    }
    if (newItems.empty()) {
        return;
    }
    m_receivedItems.reserve(m_receivedItems.size() + newItems.size());
    // Iterate through new items to add them
    for (auto item : newItems) {
        // This works because operator[] will create missing keys with 0
        ++m_receivedItemMap[item.item];
        m_receivedItems.push_back(item);
    }
}

void TrackerClient::updateCheckedLocations(const std::vector<location_id_t>& newLocations, bool reset) {
    if (reset) {
        m_checkedLocationSet.clear();
    }
    m_checkedLocationSet.insert(newLocations.cbegin(), newLocations.cend());
}

void GameClient::onReceivedItems(const packets::ReceivedItems& receivedItems) {
    std::lock_guard lock(m_mutex);
    updateReceivedItems(receivedItems.items);
    // But also add them to our deque
}

void GameClient::pushMultipleItems(const std::vector<packets::NetworkItem>& newItems) {
    m_itemDeque.insert(m_itemDeque.end(), newItems.cbegin(), newItems.cend());
}

/// <summary>
/// Pops a single item off the deque. Many games can only process receiving a single item at
/// a time and will need to wait before they can process the next one anyway. As this locks
/// the mutex, repeatedly calling popItem() in the same thread is slow!
/// </summary>
/// <returns></returns>
std::optional<packets::NetworkItem> GameClient::popItem() {
    std::lock_guard lock(m_mutex);
    if (m_itemDeque.empty()) {
        return std::nullopt;
    }
    packets::NetworkItem item = m_itemDeque.front();
    m_itemDeque.pop_front();
    m_currentIndex++;
    return item;
}

std::vector<packets::NetworkItem> GameClient::popMultipleItems(size_t count) {
    std::lock_guard lock(m_mutex);
    std::vector<packets::NetworkItem> result;
    count = std::min(m_itemDeque.size(), count);
    if (count == 0) {
        // No items to send (or popMultipleItems(0), which - why?)
        return result;
    }
    result.reserve(count);
    auto itemIt = m_itemDeque.begin();
    while (itemIt != m_itemDeque.end() && count > 0) {
        result.push_back(*itemIt);
        ++itemIt;
        --count;
    }
    // Remove the items that will be returned
    m_itemDeque.erase(m_itemDeque.begin(), itemIt);
    return result;
}

}