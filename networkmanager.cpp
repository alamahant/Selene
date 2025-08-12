#include "networkmanager.h"
#include<QMessageBox>
#include<QThread>
#include<QSystemTrayIcon>
#include<QStyle>
#include<QApplication>
#include <QFile>
#include <QTimer>
#include <QPointer>
#include <QFileInfo>
#include <QStandardPaths>
#include "constants.h"
#include"Notification.h"

NetworkManager::NetworkManager(QObject *parent)
    : QObject{parent}
{
    server = new QTcpServer(this);
    clientSocket = nullptr;
    contactManager = new ContactManager(this);
    connectedPeerAddress = "";
    messageBuffer.clear();

    connect(server, &QTcpServer::newConnection,
            this, &NetworkManager::handleNewConnection);
}

NetworkManager::~NetworkManager() {

    cleanup();
}

bool NetworkManager::initialize(const QString& onionAddress) {
    myOnionAddress = onionAddress;
    Logger::log(Logger::INFO, "Initializing network with address: " + onionAddress);

    // Create a contact for ourselves if it doesn't exist
    QString selfName = contactManager->getFriendlyName(onionAddress);
    if (selfName == onionAddress || !selfName.contains("Me")) {
        Contact selfContact;
        selfContact.onionAddress = onionAddress;
        selfContact.friendlyName = "Me";
        contactManager->addContact(selfContact);
        Logger::log(Logger::INFO, "Added self as contact with onion: " + onionAddress);
    }

    QNetworkProxy proxy;
    proxy.setType(QNetworkProxy::Socks5Proxy);
    proxy.setHostName("127.0.0.1");
    //proxy.setPort(9050);
    proxy.setPort(DefaultProxyPort);

    QNetworkProxy::setApplicationProxy(proxy);
    Logger::log(Logger::INFO, "SOCKS5 proxy configured");

    //bool success = server->listen(QHostAddress::LocalHost, 8080);
    bool success = server->listen(QHostAddress::LocalHost, DefaultChatServicePort);

    Logger::log(Logger::INFO, "Server listening status: " + QString::number(success));
    return success;
}

void NetworkManager::handleNewConnection() {

    if (!availableForChat) {
        // Accept the pending connection and immediately close it
        QTcpSocket* socket = server->nextPendingConnection();
        if (socket) {
            socket->disconnectFromHost();
            socket->deleteLater();
        }
        return;
    }

    QTcpSocket* socket = server->nextPendingConnection();
    if (!socket) {
        Logger::log(Logger::ERROR, "Invalid socket in handleNewConnection");
        return;
    }

    Logger::log(Logger::INFO, "New incoming connection from: " + socket->peerAddress().toString());

    // Create a temporary peer entry with a placeholder address
    // We'll update this with the real onion address after handshake
    QString tempAddress = "temp_" + QString::number(QDateTime::currentMSecsSinceEpoch());

    // Create peer state
    PeerState newPeer;
    newPeer.socket = socket;
    newPeer.onionAddress = tempAddress;  // Temporary, will be updated after handshake
    newPeer.isConnected = true;
    newPeer.lastSeen = QDateTime::currentDateTime();
    newPeer.connectionInitiator = false;  // We did not initiate this connection
    peers[tempAddress] = newPeer;

    // Connect signals
    connect(socket, &QTcpSocket::disconnected, [this, socket, tempAddress]() {
        Logger::log(Logger::INFO, "Incoming connection disconnected");

        // Find the actual peer address (it might have been updated after handshake)
        QString peerAddress = tempAddress;
        QVariant peerProperty = socket->property("peer_onion");
        if (peerProperty.isValid()) {
            peerAddress = peerProperty.toString();
        }

        // Update peer state
        if (peers.contains(peerAddress)) {
            peers[peerAddress].isConnected = false;
            peers[peerAddress].lastSeen = QDateTime::currentDateTime();
            peers.remove(peerAddress);
        }

        // If this was the active connection, clear it
        if (socket == clientSocket) {
            clientSocket = nullptr;
            setConnectedPeerAddress("");
            //emit connectionStatusChanged(false);
            emit connectionStatusChanged(false, peerAddress);  // Using peerAddress instead of tempAddress since it's updated after handshake
        }

        socket->deleteLater();

        emit peerDisconnected(peerAddress);
        emit peerDisconnected(); // For backward compatibility
        playSound(SoundType::Disconnected, play);


    });

    connect(socket, &QTcpSocket::errorOccurred, [this, socket, tempAddress](QAbstractSocket::SocketError error) {
        QString errorMsg = "Socket error on incoming connection: " + socket->errorString();
        Logger::log(Logger::ERROR, errorMsg);

        // Find the actual peer address (it might have been updated after handshake)
        QString peerAddress = tempAddress;
        QVariant peerProperty = socket->property("peer_onion");
        if (peerProperty.isValid()) {
            peerAddress = peerProperty.toString();
        }

        // Clean up on error
        if (peers.contains(peerAddress)) {
            peers.remove(peerAddress);
        }

        if (socket == clientSocket) {
            clientSocket = nullptr;
            setConnectedPeerAddress("");
        }

        socket->deleteLater();

        emit networkError(socket->errorString());
    });

    connect(socket, &QTcpSocket::readyRead, this, &NetworkManager::handleIncomingData);

    // Set as active connection
    clientSocket = socket;

    // Send handshake to identify ourselves
    sendHandshake(socket);

    //emit connectionStatusChanged(true);
    emit connectionStatusChanged(true, tempAddress);
    playSound(SoundType::MessageReceived, play);

}

//CONNECT TO PEER ASYNCHRONOUS VERSION
bool NetworkManager::connectToPeer(const QString& peerAddress) {

    Logger::log(Logger::INFO, "Starting connection process to: " + peerAddress);
    Logger::log(Logger::INFO, "Local onion: " + myOnionAddress);

    if (peers.contains(peerAddress) &&
        peers[peerAddress].socket &&
        peers[peerAddress].socket->state() == QAbstractSocket::ConnectedState) {
        Logger::log(Logger::INFO, "Already connected to peer: " + peerAddress);

        // Get friendly name for the peer, fallback to onion address if not set
        QString friendlyName = getFriendlyName(peerAddress);
        if (friendlyName.isEmpty())
            friendlyName = peerAddress;

        Notification("Connection Info", "Already connected to peer: " + friendlyName);

        setActiveConnection(peerAddress);
        emit peerConnected(peerAddress);

        return true;
    }

    QTcpSocket* newSocket = new QTcpSocket(this);
    PeerState newPeer;
    newPeer.socket = newSocket;
    newPeer.onionAddress = peerAddress;
    newPeer.isConnected = false;
    newPeer.lastSeen = QDateTime::currentDateTime();
    newPeer.connectionInitiator = true;
    peers[peerAddress] = newPeer;

    if (newPeer.connectionInitiator) {
        QTimer* keepAliveTimer = new QTimer(newSocket);
        keepAliveTimer->setInterval(60000); // 60 seconds
        connect(keepAliveTimer, &QTimer::timeout, [this, newSocket, peerAddress]() {
            if (newSocket->state() == QAbstractSocket::ConnectedState) {
                Logger::log(Logger::DEBUG, "Sending keepalive to: " + peerAddress);
                sendPing(newSocket);
            }
        });
        connect(newSocket, &QTcpSocket::disconnected, keepAliveTimer, &QTimer::stop);
        connect(newSocket, &QTcpSocket::disconnected, keepAliveTimer, &QTimer::deleteLater);
        keepAliveTimer->start();
    }

    connect(newSocket, &QTcpSocket::connected, [this, newSocket, peerAddress]() {
        Logger::log(Logger::INFO, "Outgoing connection established to: " + peerAddress);
        Notification("Connection Established", "Outgoing connection established to: " + peerAddress);


        if (peers.contains(peerAddress)) {
            peers[peerAddress].isConnected = true;
            peers[peerAddress].lastSeen = QDateTime::currentDateTime();

        }
        clientSocket = newSocket;
        setConnectedPeerAddress(peerAddress);
        sendHandshake(newSocket);
        emit connectionStatusChanged(true, peerAddress);
        emit peerConnected(peerAddress);
        playSound(SoundType::MessageReceived, play);
    });

    connect(newSocket, &QTcpSocket::disconnected, [this, newSocket, peerAddress]() {

        Logger::log(Logger::INFO, "Outgoing connection disconnected from: " + peerAddress);
        Notification("Connection Disconnected", "Outgoing connection disconnected from: " + peerAddress);


        cleanupConnection(peerAddress);
        emit peerDisconnected(peerAddress);
        emit peerDisconnected();
        emit connectionStatusChanged(false, peerAddress);
        playSound(SoundType::Disconnected, play);

    });
    connect(newSocket, &QTcpSocket::errorOccurred, [this, newSocket, peerAddress](QAbstractSocket::SocketError error) {
        QString errorMsg = "Socket error on connection to " + peerAddress + ": " + newSocket->errorString();
        Logger::log(Logger::ERROR, errorMsg);

        Notification("Connection Error", errorMsg, 3000, QSystemTrayIcon::Critical);

        if (peers.contains(peerAddress)) {
            peers[peerAddress].isConnected = false;
            peers.remove(peerAddress);
        }
        if (newSocket == clientSocket) {
            clientSocket = nullptr;
            setConnectedPeerAddress("");
        }
        newSocket->deleteLater(); // Safe deletion
        emit networkError(newSocket->errorString());
    });

    connect(newSocket, &QTcpSocket::readyRead, this, &NetworkManager::handleIncomingData);

    Logger::log(Logger::INFO, "Attempting connection to: " + peerAddress);
    newSocket->connectToHost(peerAddress, 80);

    // 30-second connection timeout

    QTimer* connectTimeout = new QTimer(newSocket);
    connectTimeout->setSingleShot(true);
    connectTimeout->setInterval(20000); // 20 seconds

    // If connected before timeout, stop and delete the timer
    connect(newSocket, &QTcpSocket::connected, connectTimeout, &QTimer::stop);
    connect(newSocket, &QTcpSocket::connected, connectTimeout, &QTimer::deleteLater);

    // If timeout fires before connection, abort and clean up
    connect(connectTimeout, &QTimer::timeout, [this, newSocket, peerAddress]() {
        Logger::log(Logger::WARNING, "Connection to " + peerAddress + " timed out after 30 seconds.");
        newSocket->abort();
        // show a message here
        QMessageBox::warning(
            nullptr,
            "Unable to Connect",
            QString("Unable to connect to %1.\n\nPlease request a new Tor circuit and try again.").arg(peerAddress)
            );
        Notification("Unable to Connect", QString("Unable to connect to %1.\n\nPlease request a new Tor circuit and try again.")
                                              .arg(peerAddress), 3000, QSystemTrayIcon::Warning);
    });

    connectTimeout->start();
    // Immediately return true: connection is now asynchronous
    return true;
}




void NetworkManager::disconnectFromPeer(const QString& peerAddress) {
    disconnectRequested[peerAddress] = true;

    Logger::log(Logger::DEBUG, "Deliberate disconnect initiated for peer: " + peerAddress);

    // Only disconnect if the peer exists
    if (peers.contains(peerAddress)) {
        PeerState& peer = peers[peerAddress];
        if (peer.socket) {
            peer.socket->disconnect(); // Disconnect signals
            peer.socket->abort();      // Force close
            peer.socket->deleteLater();
        }
        peers.remove(peerAddress);

        // If this was the active connection, clear it
        if (connectedPeerAddress == peerAddress) {
            setConnectedPeerAddress("");
            clientSocket = nullptr;
        }

        emit peerDisconnected(peerAddress);
        emit connectionStatusChanged(false, peerAddress);
        playSound(SoundType::Disconnected, play);
    } else {
        Logger::log(Logger::WARNING, "Attempted to disconnect unknown peer: " + peerAddress);
    }
}

void NetworkManager::sendMessage(const QString& message) {
    if (!connectedPeerAddress.isEmpty()) {
        sendMessageToPeer(connectedPeerAddress, message);
    } else {
        Logger::log(Logger::ERROR, "Cannot send message - no active connection");
    }
}

void NetworkManager::sendHandshake(QTcpSocket* socket)
{
    QJsonObject handshake;
    handshake["type"] = "handshake";
    handshake["onion"] = myOnionAddress;

    QJsonDocument doc(handshake);
    //QByteArray data = doc.toJson();
    QByteArray data = doc.toJson(QJsonDocument::Compact) + "\n";

    socket->write(data);
    socket->flush();
    Logger::log(Logger::INFO, "Handshake sent with onion: " + myOnionAddress);
}


void NetworkManager::handleIncomingData() {

    QTcpSocket* socket = qobject_cast<QTcpSocket*>(sender());

    if (!socket) {
        Logger::log(Logger::ERROR, "Invalid socket in handleIncomingData");
        return;
    }

    PeerState* peer = nullptr;
    for (auto it = peers.begin(); it != peers.end(); ++it) {
        if (it.value().socket == socket) {
            peer = &it.value();
            break;
        }
    }

    if (!peer) {
        Logger::log(Logger::ERROR, "No peer found for socket");
        return;
    }

    // Append new data to the buffer
    QByteArray newData = socket->readAll();
    Logger::log(Logger::DEBUG, "Received data: " + QString(newData));
    peer->messageBuffer.append(newData);

    // Process all complete messages (split by '\n')
    while (true) {
        int newlineIndex = peer->messageBuffer.indexOf('\n');
        if (newlineIndex == -1)
            break; // No complete message yet

        QByteArray messageData = peer->messageBuffer.left(newlineIndex);
        peer->messageBuffer.remove(0, newlineIndex + 1);

        if (messageData.trimmed().isEmpty())
            continue; // Skip empty lines

        QJsonParseError parseError;
        QJsonDocument doc = QJsonDocument::fromJson(messageData, &parseError);

        // If we have a complete valid JSON message
        if (parseError.error == QJsonParseError::NoError) {
            if (doc.isObject()) {
                QJsonObject message = doc.object();
                QString type = message["type"].toString();

                if (type == "handshake") {
                    QString peerOnion = message["onion"].toString();

                    //protect against double connection
                    if (peers.contains(peerOnion) && peers[peerOnion].isConnected && peers[peerOnion].socket != socket) {
                        Logger::log(Logger::INFO, "Duplicate connection from " + peerOnion + " blocked.");

                        // Get friendly name for the peer, fallback to onion if not set
                        QString friendlyName = getFriendlyName(peerOnion);
                        if (friendlyName.isEmpty())
                            friendlyName = peerOnion;

                        QMessageBox::warning(
                            nullptr,
                            "Double Connection Detected",
                            "A double connection was detected with peer: " + friendlyName +
                                "\n\nThis can happen if both peers try to connect at the same time. "
                                "Usually, this is harmless and messaging will work as normal.\n\n"
                                "If you experience any issues sending or receiving messages, "
                                "please disconnect from this peer and wait for their incoming connection, "
                                "or agree on who should initiate the connection in the future."
                            );
                        // socket->disconnectFromHost();
                        // socket->deleteLater();
                        return;
                    }

                    //
                    // Check if this is an unknown contact
                    if (!contactManager->getContacts().contains(peerOnion)) {
                        emit unknownContactMessageReceived(peerOnion);
                        playSound(SoundType::UnknownContact, play);
                    }

                    // Check if peer is blocked before processing handshake
                    if (contactManager->isBlocked(peerOnion)) {
                        Logger::log(Logger::INFO, "Rejecting connection from unavailable peer: " + peerOnion);
                        socket->disconnectFromHost();
                        return;
                    }

                    Logger::log(Logger::INFO, "Handshake received from: " + peerOnion);

                    // Store the onion address as a property on the socket
                    socket->setProperty("peer_onion", peerOnion);

                    // Find the temporary peer entry and update it with the real onion address
                    QString tempKey = "";
                    for (auto it = peers.begin(); it != peers.end(); ++it) {
                        if (it.value().socket == socket) {
                            tempKey = it.key();
                            break;
                        }
                    }
                    if (!tempKey.isEmpty() && tempKey != peerOnion) {
                        // Remove the temporary entry
                        PeerState state = peers[tempKey];
                        peers.remove(tempKey);
                        // Update with real onion address
                        state.onionAddress = peerOnion;
                        peers[peerOnion] = state;
                    }

                    // Set as active connection
                    clientSocket = socket;
                    setConnectedPeerAddress(peerOnion);

                    // Send handshake back if we haven't already
                    if (!socket->property("handshake_sent").toBool()) {
                        sendHandshake(socket);
                        socket->setProperty("handshake_sent", true);
                    }

                    // Emit signals for peer identification and connection
                    emit peerIdentified(peerOnion);
                    emit peerConnected(peerOnion);

                    if(!peer->connectionInitiator)
                        playSound(SoundType::MessageReceived, play);
                }
                else if (type == "message") {
                    QString content = message["content"].toString();

                    // Find which peer sent this message
                    QString senderOnion;
                    // First try to get from socket property
                    QVariant peerProperty = socket->property("peer_onion");
                    if (peerProperty.isValid()) {
                        senderOnion = peerProperty.toString();
                    }
                    // If not found in property, try to find in peers map
                    if (senderOnion.isEmpty()) {
                        for (auto it = peers.begin(); it != peers.end(); ++it) {
                            if (it.value().socket == socket) {
                                senderOnion = it.key();
                                break;
                            }
                        }
                    }
                    // If we still don't have a sender, use the connected peer address
                    if (senderOnion.isEmpty() && !connectedPeerAddress.isEmpty()) {
                        senderOnion = connectedPeerAddress;
                        Logger::log(Logger::INFO, "Using connectedPeerAddress for sender: " + senderOnion);
                    }
                    // Log the message reception
                    Logger::log(Logger::INFO, "Message received from " + senderOnion + ": " + content);
                    // Emit the signal with sender and content
                    emit messageReceived(senderOnion, content);
                    playSound(SoundType::MessageReceived, play);
                }
            }
        } else {
            // If we couldn't parse the message, log the error but don't clear the buffer
            // It might be an incomplete message that will be completed with more data
            Logger::log(Logger::DEBUG, "Incomplete message or parse error: " + parseError.errorString());
        }
    }
}




void NetworkManager::addContact(const QString& onion, const QString& friendlyName) {
    Contact newContact;
    newContact.onionAddress = onion;
    newContact.friendlyName = friendlyName;
    contactManager->addContact(newContact);
}





QString NetworkManager::getFriendlyName(const QString& onion) const
{
    return contactManager->getFriendlyName(onion);
}




bool NetworkManager::isContactBlocked(const QString& onion) const
{
    return contactManager->isBlocked(onion);
}

void NetworkManager::setContactBlocked(const QString& onion, bool blocked)
{
    contactManager->setBlocked(onion, blocked);
}

QMap<QString, Contact> NetworkManager::getContacts() const
{
    return contactManager->getContacts();
}




void NetworkManager::deleteContact(const QString& onion)
{
    contactManager->deleteContact(onion);
}

void NetworkManager::checkPeerAvailability(const QString& onionAddress) {
    Logger::log(Logger::INFO, "Connecting to peer: " + onionAddress);

    // Attempt to connect to the peer
    bool connected = connectToPeer(onionAddress);

    // Notify about the connection result
    emit peerAvailabilityResult(onionAddress, connected);
}


ContactManager *NetworkManager::getContactManager() const
{
    return contactManager;
}




QStringList NetworkManager::getConnectedPeers() const {
    QStringList connectedPeers;
    for (auto it = peers.begin(); it != peers.end(); ++it) {
        if (it.value().isConnected && it.value().socket &&
            it.value().socket->state() == QAbstractSocket::ConnectedState) {
            connectedPeers.append(it.key());
        }
    }
    return connectedPeers;
}

bool NetworkManager::isPeerConnected(const QString& peerAddress) const {
    return peers.contains(peerAddress) &&
           peers[peerAddress].isConnected &&
           peers[peerAddress].socket &&
           peers[peerAddress].socket->state() == QAbstractSocket::ConnectedState;
}

void NetworkManager::setActiveConnection(const QString& peerAddress) {
    if (peers.contains(peerAddress) &&
        peers[peerAddress].isConnected &&
        peers[peerAddress].socket &&
        peers[peerAddress].socket->state() == QAbstractSocket::ConnectedState) {

        clientSocket = peers[peerAddress].socket;
        setConnectedPeerAddress(peerAddress);
        Logger::log(Logger::INFO, "Active connection set to: " + peerAddress);
        emit activeConnectionChanged(peerAddress);
    } else {
        Logger::log(Logger::ERROR, "Cannot set active connection - peer not connected: " + peerAddress);
    }
}

QString NetworkManager::getActiveConnection() const {
    return connectedPeerAddress;
}

void NetworkManager::sendMessageToPeer(const QString& peerAddress, const QString& message) {
    Logger::log(Logger::INFO, "Attempting to send message to " + peerAddress + ": " + message);

    if (peers.contains(peerAddress) &&
        peers[peerAddress].isConnected &&
        peers[peerAddress].socket &&
        peers[peerAddress].socket->state() == QAbstractSocket::ConnectedState) {

        QJsonObject messageObj;
        messageObj["type"] = "message";
        messageObj["content"] = message;
        QJsonDocument doc(messageObj);
        //QByteArray data = doc.toJson();

        // to separate messages perfectly
        QByteArray data = doc.toJson(QJsonDocument::Compact) + "\n";

        qint64 bytesSent = peers[peerAddress].socket->write(data);
        peers[peerAddress].socket->flush();
        Logger::log(Logger::INFO, "Bytes sent to " + peerAddress + ": " + QString::number(bytesSent));
    } else {
        Logger::log(Logger::ERROR, "Cannot send message - peer not connected: " + peerAddress);
    }
    playSound(SoundType::MessageSent, play);
}

void NetworkManager::cleanupConnection(const QString& peerAddress) {
    if (peers.contains(peerAddress)) {
        PeerState& peer = peers[peerAddress];
        peer.messageBuffer.clear();
        if (peer.socket) {
            peer.socket->disconnect();
            if (peer.socket->state() != QAbstractSocket::UnconnectedState) {
                peer.socket->abort();
                peer.socket->waitForDisconnected(1000);
            }
            delete peer.socket;
        }
        peers.remove(peerAddress);
    }
    if (connectedPeerAddress == peerAddress && clientSocket) {
        clientSocket->disconnect();
        if (clientSocket->state() != QAbstractSocket::UnconnectedState) {
            clientSocket->abort();
            clientSocket->waitForDisconnected(1000);
        }
        delete clientSocket;
        clientSocket = nullptr;
        setConnectedPeerAddress("");
    }
}

void NetworkManager::cleanup() {
    Logger::log(Logger::INFO, "Performing network cleanup before application exit");
    if (server) {
        server->close();
    }

    for (auto it = peers.begin(); it != peers.end(); ++it) {
        QString address = it.key();
        Logger::log(Logger::INFO, "Cleaning up connection to: " + address);
        cleanupConnection(address);
    }
    peers.clear();

    if (clientSocket) {
        clientSocket->disconnect();
        clientSocket->abort();
        clientSocket->deleteLater();
        clientSocket = nullptr;
    }
    setConnectedPeerAddress("");
    Logger::log(Logger::INFO, "Network cleanup completed");
}




void NetworkManager::sendPing(QTcpSocket* socket) {
    QJsonObject pingObj;
    pingObj["type"] = "ping";
    QJsonDocument doc(pingObj);
    //QByteArray pingMessage = doc.toJson();
    // Properly isolate ping messages
    QByteArray pingMessage = doc.toJson(QJsonDocument::Compact) + "\n";

    socket->write(pingMessage);
}

void NetworkManager::verifyHiddenService() {
    QTcpSocket* testSocket = new QTcpSocket(this);
    testSocket->connectToHost(myOnionAddress, 80);

    connect(testSocket, &QTcpSocket::connected, [this]() {
        Logger::log(Logger::INFO, "Hidden service verified active");
        emit initProgress(90, "Hidden service ready");
    });

    connect(testSocket, &QTcpSocket::errorOccurred, [this](QAbstractSocket::SocketError error) {
        Logger::log(Logger::WARNING, "Hidden service not yet reachable");
        // Retry after delay
        QTimer::singleShot(5000, this, &NetworkManager::verifyHiddenService);
    });

    testSocket->deleteLater();
}

void NetworkManager::attemptReconnect(const QString& peerAddress, int attemptsLeft)
{
    if (attemptsLeft <= 0) {
        Logger::log(Logger::ERROR, "Failed to reconnect to peer: " + peerAddress + " after maximum attempts.");
        return;
    }

    Logger::log(Logger::INFO, "Attempting to reconnect to peer: " + peerAddress +
                                  " (" + QString::number(attemptsLeft) + " attempts left)");

    // Try to connect asynchronously
    //startedbyme = true;

    bool started = connectToPeer(peerAddress);

    // If connectToPeer fails to start, schedule another attempt after a delay
    if (!started) {
        QTimer::singleShot(2000, this, [this, peerAddress, attemptsLeft]() {
            attemptReconnect(peerAddress, attemptsLeft - 1);
        });
    }
    // If connectToPeer is asynchronous and you want to retry only on error,
    // you can trigger attemptReconnect from your errorOccurred handler instead.
}

// sound system
void NetworkManager::playSound(SoundType type, bool playIt)
{
    if (!playIt) return;

    if (!m_soundEffects.contains(type)) {
        // Create and cache the sound effect for this type
        QSoundEffect* effect = new QSoundEffect(this);
        effect->setSource(QUrl("qrc" + soundResourcePath(type)));
        effect->setVolume(0.8);
        m_soundEffects[type] = effect;
    }
    QSoundEffect* effect = m_soundEffects[type];
    // If already playing, stop and replay
    if (effect->isPlaying()) {
        effect->stop();
    }
    effect->play();
}



