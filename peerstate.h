#ifndef PEERSTATE_H
#define PEERSTATE_H
#include<QString>
#include<QDateTime>
#include<QTcpSocket>
class PeerState
{
public:

    QString onionAddress;
    QString friendlyName;
    bool isBlocked;
    QDateTime lastSeen;
    bool isConnected;
    QTcpSocket* socket;
    bool connectionInitiator; // Added this field to track who initiated the connection
    QByteArray messageBuffer;
    QByteArray binaryBuffer;

    PeerState(const QString& address = "") :
        onionAddress(address),
        isBlocked(false),
        isConnected(false),
        connectionInitiator(false), // Initialize to false by default
        messageBuffer(),  // Initialize empty buffer
        binaryBuffer() {}  // Initialize empty buffer


};

#endif // PEERSTATE_H
