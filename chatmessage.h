#ifndef CHATMESSAGE_H
#define CHATMESSAGE_H
#include<QString>
#include<QJsonObject>

struct ChatMessage {
    QString senderAddress;
    QString content;
    QDateTime timestamp;
    bool isRead;
    bool isDelivered;
    bool isFromMe;

    // --- File transfer support ---
    bool isFile = false;                // True if this is a file message
    QString fileName;                   // File name (if file message)
    qint64 fileSize = 0;                // File size in bytes
    qint64 bytesTransferred = 0;        // For progress indication
    QString fileStatus;                 // "pending", "in_progress", "complete", "failed"
    QString fileError;
    QJsonObject toJson() const;
    static ChatMessage fromJson(const QJsonObject& obj);

};


#endif // CHATMESSAGE_H
