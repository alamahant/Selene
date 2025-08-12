#include "chatsession.h"
#include<QJsonArray>

// ChatSession serialization
QJsonObject ChatSession::toJson() const {
    QJsonObject obj;
    obj["peerAddress"] = peerAddress;
    obj["peerName"] = peerName;
    obj["isActive"] = isActive;
    obj["lastActivity"] = lastActivity.toString(Qt::ISODate);

    QJsonArray msgArray;
    for (const ChatMessage& msg : messages) {
        msgArray.append(msg.toJson());
    }
    obj["messages"] = msgArray;
    return obj;
}

ChatSession ChatSession::fromJson(const QJsonObject& obj) {
    ChatSession session;
    session.peerAddress = obj.value("peerAddress").toString();
    session.peerName = obj.value("peerName").toString();
    session.isActive = obj.value("isActive").toBool();
    session.lastActivity = QDateTime::fromString(obj.value("lastActivity").toString(), Qt::ISODate);

    QJsonArray msgArray = obj.value("messages").toArray();
    for (const QJsonValue& val : msgArray) {
        session.messages.append(ChatMessage::fromJson(val.toObject()));
    }
    return session;
}
