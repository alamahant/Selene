#ifndef CHATSESSION_H
#define CHATSESSION_H
#include<QString>
#include<QJsonObject>
#include"chatmessage.h"

// Chat session structure
struct ChatSession {
    QString peerAddress;
    QString peerName;
    QList<ChatMessage> messages;
    bool isActive;
    QDateTime lastActivity;
    QJsonObject toJson() const;

    static ChatSession fromJson(const QJsonObject& obj);

};

#endif // CHATSESSION_H
