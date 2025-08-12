#include "chatmessage.h"

QJsonObject ChatMessage::toJson() const {
    QJsonObject obj;
    obj["senderAddress"] = senderAddress;
    obj["content"] = content;
    obj["timestamp"] = timestamp.toString(Qt::ISODate);
    obj["isRead"] = isRead;
    obj["isDelivered"] = isDelivered;
    obj["isFromMe"] = isFromMe;
    return obj;
}

ChatMessage ChatMessage::fromJson(const QJsonObject& obj) {
    ChatMessage msg;
    msg.senderAddress = obj.value("senderAddress").toString();
    msg.content = obj.value("content").toString();
    msg.timestamp = QDateTime::fromString(obj.value("timestamp").toString(), Qt::ISODate);
    msg.isRead = obj.value("isRead").toBool();
    msg.isDelivered = obj.value("isDelivered").toBool();
    msg.isFromMe = obj.value("isFromMe").toBool();
    return msg;
}
