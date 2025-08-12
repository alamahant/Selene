#ifndef NETWORKMANAGER_H
#define NETWORKMANAGER_H

#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QNetworkProxy>
#include "logger.h"
#include "peerstate.h"
#include <QMediaPlayer>
#include <QRandomGenerator>
#include <QByteArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include "contactmanager.h"
#include <QTimer>
#include<QProgressDialog>
#include"constants.h"
#include<QSoundEffect>
#include<QMap>

class NetworkManager : public QObject
{
    Q_OBJECT
public:
    explicit NetworkManager(QObject *parent = nullptr);
    ~NetworkManager();

    bool initialize(const QString& onionAddress);
    bool connectToPeer(const QString& peerAddress);
    void sendMessage(const QString& message);
    void sendMessageToPeer(const QString& peerAddress, const QString& message);
    void sendHandshake(QTcpSocket* socket);
    void addContact(const QString& onion, const QString& friendlyName);
    QString getFriendlyName(const QString& onion) const;
    bool isContactBlocked(const QString& onion) const;
    void setContactBlocked(const QString& onion, bool blocked);
    QMap<QString, Contact> getContacts() const;
    void sendFile(const QJsonObject& fileMessage);
    void resetTransfer();
    void deleteContact(const QString& onion);

    // Methods for managing active connection
    void setActiveConnection(const QString& peerAddress);
    QString getActiveConnection() const;
    QStringList getConnectedPeers() const;
    bool isPeerConnected(const QString& peerAddress) const;

public slots:
    void handleNewConnection();
    void handleIncomingData();
    void disconnectFromPeer(const QString& peerAddress = QString());



signals:
    void messageReceived(const QString& senderOnion, const QString& message);
    void peerConnected(const QString& address);
    void connectionStatusChanged(bool connected, const QString& peerAddress = QString());
    void activeConnectionChanged(const QString& peerAddress);
    void networkError(const QString& error);
    //void peerDisconnected();
    void peerDisconnected(const QString& peerAddress = QString());

    void peerIdentified(const QString& peerOnion);
    void fileReceived(const QString& fileName, const QByteArray& fileData);
    void transferProgress(int percentComplete);
    void peerAvailabilityResult(const QString& onionAddress, bool connected);

private:
    QTcpServer* server;
    QTcpSocket* clientSocket; // Now represents the active connection
    QString myOnionAddress;
    QTcpSocket* serverSocket = nullptr;
    QMap<QString, PeerState> peers;  // onion address -> peer state
    QMediaPlayer* connectionAlert;
    ContactManager* contactManager;
    QByteArray fileBuffer;
    qint64 totalBytes;
    qint64 bytesWritten;
    QByteArray receivedData;
    qint64 expectedFileSize;
    QString currentFileName;
    QString connectedPeerAddress; // Now represents the active peer
    QByteArray messageBuffer;
    QTimer* keepAliveTimer;
    const int KEEPALIVE_INTERVAL = 30000; // 30 seconds

public:
    void checkPeerAvailability(const QString& onionAddress);
    QString getConnectedPeerAddress() const { return connectedPeerAddress; }
    void setConnectedPeerAddress(const QString& address) {
        connectedPeerAddress = address;
        Logger::log(Logger::INFO, "Connected peer address set to: " + address);
    }
    ContactManager *getContactManager() const;
    void cleanup();
private:
    void cleanupConnection(const QString& peerAddress);
    void sendPing(QTcpSocket* socket);

    void verifyHiddenService();
signals:
    void initProgress(int percent, QString status);
    void unknownContactMessageReceived(const QString& senderAddress);

private:
    // reconnect functionality
    QMap<QString, bool> disconnectRequested; // Tracks if disconnect was intentional
    void attemptReconnect(const QString& peerAddress, int attemptsLeft);
    //sound system
    void playSound(SoundType type, bool playIt);
    bool play = true;
    QMap<SoundType, QSoundEffect*> m_soundEffects;
public:
    // available for chat?
    bool availableForChat = true;
    void setPlay(bool playIt){
        play = playIt;
    }
};

#endif // NETWORKMANAGER_H

