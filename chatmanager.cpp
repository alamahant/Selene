#include "chatmanager.h"

#include <QTextDocument>
#include <QTextCursor>
#include <QTextBlock>
#include <QTextBlockFormat>
#include <QTextCharFormat>
#include <QBrush>
#include <QColor>
#include <QDateTime>
#include<QScrollBar>
#include<QTimer>
#include<QFile>
#include<QDir>
#include<QJsonArray>
#include<QJsonDocument>
#include<QJsonObject>
#include"constants.h"


ChatManager::ChatManager(QObject *parent) : QObject(parent) {
}

void ChatManager::createSession(const QString& peerAddress, const QString& peerName) {

    if (!chatSessions.contains(peerAddress)) {
        ChatSession session;
        session.peerAddress = peerAddress;
        session.peerName = peerName;
        session.isActive = false;
        session.lastActivity = QDateTime::currentDateTime();
        chatSessions[peerAddress] = session;

        emit sessionCreated(peerAddress);
    } else {
    }
}

void ChatManager::activateSession(const QString& peerAddress) {

    if (chatSessions.contains(peerAddress)) {
        // Deactivate current active session
        if (!activeSessionPeer.isEmpty() && chatSessions.contains(activeSessionPeer)) {
            chatSessions[activeSessionPeer].isActive = false;
        }

        // Activate new session
        chatSessions[peerAddress].isActive = true;
        chatSessions[peerAddress].lastActivity = QDateTime::currentDateTime();
        activeSessionPeer = peerAddress;

        emit sessionActivated(peerAddress);
    } else {
    }
}

void ChatManager::closeSession(const QString& peerAddress) {

    if (chatSessions.contains(peerAddress)) {
        if (activeSessionPeer == peerAddress) {
            activeSessionPeer.clear();
        }

        chatSessions.remove(peerAddress);
        emit sessionClosed(peerAddress);
    } else {
    }
}

bool ChatManager::hasActiveSession() const {
    bool hasActive = !activeSessionPeer.isEmpty() && chatSessions.contains(activeSessionPeer);
    return hasActive;
}

QString ChatManager::getActiveSessionPeer() const {
    return activeSessionPeer;
}

QStringList ChatManager::getAllSessionPeers() const {
    QStringList peers = chatSessions.keys();
    return peers;
}

void ChatManager::addMessage(const QString& peerAddress, const QString& content, bool isFromMe) {


    if (chatSessions.contains(peerAddress)) {
        ChatMessage message;
        message.senderAddress = isFromMe ? "me" : peerAddress; // Ensure consistent sender ID
        message.content = content;
        message.timestamp = QDateTime::currentDateTime();
        message.isRead = isFromMe; // Own messages are always "read"
        message.isDelivered = isFromMe; // Own messages are always "delivered"
        message.isFromMe = isFromMe;

        chatSessions[peerAddress].messages.append(message);
        chatSessions[peerAddress].lastActivity = message.timestamp;



        emit messageAdded(peerAddress, message);
    } else {
    }
}

QList<ChatMessage> ChatManager::getMessages(const QString& peerAddress, int count) {

    if (chatSessions.contains(peerAddress)) {
        QList<ChatMessage> messages = chatSessions[peerAddress].messages;

        if (count > 0 && count < messages.size()) {
            return messages.mid(messages.size() - count);
        }

        return messages;
    }

    return QList<ChatMessage>();
}

void ChatManager::markAsRead(const QString& peerAddress) {

    if (chatSessions.contains(peerAddress)) {
        int unreadCount = 0;
        for (int i = 0; i < chatSessions[peerAddress].messages.size(); i++) {
            if (!chatSessions[peerAddress].messages[i].isRead) {
                unreadCount++;
                chatSessions[peerAddress].messages[i].isRead = true;
            }
        }
    } else {
    }
}


// Helper function to format message time
QString ChatManager::formatMessageTime(const QDateTime& timestamp) {
    return timestamp.time().toString("hh:mm");
}



QMap<QString, ChatSession>& ChatManager::getChatSessions()
{
    return chatSessions;
}





QString ChatManager::formatMessageHTML(const ChatMessage& message, bool consecutive) {
    QString messageClass = message.isFromMe ? "my-message" : "peer-message";
    QString consecutiveClass = consecutive ? " consecutive" : "";


    QString html = QString("<div class='message-container %1%2'>").arg(messageClass, consecutiveClass);

    // Message bubble with content
    html += QString("<div class='message-bubble'>%1</div>").arg(message.content);

    // Time and status
    html += "<div class='message-time'>";
    html += getTimeString(message.timestamp);

    // Add status indicators for outgoing messages
    if (message.isFromMe) {
        html += "<span class='message-status'>";
        if (message.isDelivered) {
            html += "✓"; // Single check for delivered
            if (message.isRead) {
                html += "✓"; // Double check for read
            }
        }
        html += "</span>";
    }

    html += "</div></div>";

    return html;
}

QString ChatManager::getTimeString(const QDateTime& timestamp) {
    return timestamp.toString("hh:mm");
}

QString ChatManager::getDateHeader(const QDateTime& timestamp, const QDate& prevDate) {
    // If no previous date or different day, show date header
    if (prevDate.isNull() || prevDate != timestamp.date()) {
        QString dateText;
        QDate today = QDate::currentDate();
        QDate yesterday = today.addDays(-1);
        QDate messageDate = timestamp.date();

        if (messageDate == today) {
            dateText = "Today";
        } else if (messageDate == yesterday) {
            dateText = "Yesterday";
        } else {
            dateText = messageDate.toString("MMMM d, yyyy");
        }

        return QString("<div class='date-header'>%1</div>").arg(dateText);
    }

    return "";
}

void ChatManager::renderMessagesToScrollArea(const QString& peerAddress, QScrollArea* scrollArea, int fontSize) {

    if (!chatSessions.contains(peerAddress)) {
        return;
    }

    // Get or create the chat widget
    QWidget* chatWidget = scrollArea->widget();
    if (!chatWidget) {
        chatWidget = new QWidget();
        scrollArea->setWidget(chatWidget);
        scrollArea->setWidgetResizable(true);
    }

    // Clear existing layout
    if (chatWidget->layout()) {
        QLayoutItem* item;
        while ((item = chatWidget->layout()->takeAt(0)) != nullptr) {
            delete item->widget();
            delete item;
        }
        delete chatWidget->layout();
    }

    // Create new layout
    QVBoxLayout* layout = new QVBoxLayout(chatWidget);
    layout->setAlignment(Qt::AlignTop);
    layout->setSpacing(3);
    layout->setContentsMargins(5, 5, 5, 5);

    // Get the chat session
    const ChatSession& session = chatSessions[peerAddress];

    // Add message widgets
    for (const ChatMessage& message : session.messages) {
        QString senderDisplayName = message.isFromMe ? "Me" : session.peerName;


        //MessageBubbleWidget* bubbleWidget = new MessageBubbleWidget(message, session.peerName);
        MessageBubbleWidget* bubbleWidget = new MessageBubbleWidget(message, senderDisplayName);
        bubbleWidget->setFontSize(fontSize); // set font size

        layout->addWidget(bubbleWidget);
    }

    // Add stretch to push messages to top
    layout->addStretch();


    // Scroll to bottom after a short delay
    QTimer::singleShot(10, [scrollArea]() {
        scrollArea->verticalScrollBar()->setValue(scrollArea->verticalScrollBar()->maximum());
    });

}


bool ChatManager::saveToFile(const QString& filePath) const {
    QJsonArray sessionArray;
    for (const ChatSession& session : chatSessions) {
        sessionArray.append(session.toJson());
    }
    QJsonDocument doc(sessionArray);
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly))
        return false;
    file.write(doc.toJson());
    file.close();
    return true;
}

bool ChatManager::loadFromFile(const QString& filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly))
        return false;
    QByteArray data = file.readAll();
    file.close();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isArray())
        return false;
    chatSessions.clear();
    QJsonArray sessionArray = doc.array();
    for (const QJsonValue& val : sessionArray) {
        ChatSession session = ChatSession::fromJson(val.toObject());
        chatSessions[session.peerAddress] = session;
    }
    return true;
}

// File Transfers
void ChatManager::addFileMessage(const QString& peerAddress, const QString& fileName, qint64 fileSize, bool isFromMe)
{
    if (!chatSessions.contains(peerAddress)) {
        qWarning() << "ChatManager::addFileMessage - No session for peer:" << peerAddress;
        return;
    }

    ChatMessage msg;
    msg.senderAddress = isFromMe ? "me" : peerAddress;
    msg.content = QString(); // No text content for file messages
    msg.timestamp = QDateTime::currentDateTime();
    msg.isRead = isFromMe; // Sent messages are read by default
    msg.isDelivered = false;
    msg.isFromMe = isFromMe;

    // File transfer fields
    msg.isFile = true;
    msg.fileName = fileName;
    msg.fileSize = fileSize;
    msg.bytesTransferred = 0;
    msg.fileStatus = "pending";
    msg.fileError = QString();

    chatSessions[peerAddress].messages.append(msg);

    emit fileMessageAdded(peerAddress, msg);
    emit messageAdded(peerAddress, msg); // For compatibility with existing UI
}

// Update progress for an ongoing file transfer
void ChatManager::updateFileTransferProgress(const QString& peerAddress, const QString& fileName, qint64 bytesTransferred, qint64 totalBytes)
{
    ChatMessage* msg = findFileMessage(peerAddress, fileName);
    if (!msg) {
        qWarning() << "ChatManager::updateFileTransferProgress - File message not found for" << peerAddress << fileName;
        return;
    }
    msg->bytesTransferred = bytesTransferred;
    msg->fileSize = totalBytes;
    msg->fileStatus = "in_progress";

    emit fileMessageAdded(peerAddress, *msg); // Optional: update UI
    emit messageAdded(peerAddress, *msg);     // Optional: update UI
}

// Mark a file transfer as complete
void ChatManager::markFileTransferComplete(const QString& peerAddress, const QString& fileName)
{
    ChatMessage* msg = findFileMessage(peerAddress, fileName);
    if (!msg) {
        qWarning() << "ChatManager::markFileTransferComplete - File message not found for" << peerAddress << fileName;
        return;
    }
    msg->bytesTransferred = msg->fileSize;
    msg->fileStatus = "complete";
    msg->isDelivered = true;

    emit fileMessageAdded(peerAddress, *msg);
    emit messageAdded(peerAddress, *msg);
}

// Mark a file transfer as failed
void ChatManager::markFileTransferFailed(const QString& peerAddress, const QString& fileName, const QString& error)
{
    ChatMessage* msg = findFileMessage(peerAddress, fileName);
    if (!msg) {
        qWarning() << "ChatManager::markFileTransferFailed - File message not found for" << peerAddress << fileName;
        return;
    }
    msg->fileStatus = "failed";
    msg->fileError = error;

    emit fileMessageAdded(peerAddress, *msg);
    emit messageAdded(peerAddress, *msg);
}

// Helper: Find file message in session (for progress updates)
ChatMessage* ChatManager::findFileMessage(const QString& peerAddress, const QString& fileName)
{
    if (!chatSessions.contains(peerAddress))
        return nullptr;
    for (ChatMessage& msg : chatSessions[peerAddress].messages) {
        if (msg.isFile && msg.fileName == fileName) {
            return &msg;
        }
    }
    return nullptr;
}

void ChatManager::clearHistoryForPeer(const QString& peerOnion) {
    auto it = chatSessions.find(peerOnion);
    if (it != chatSessions.end()) {
        it.value().messages.clear(); // Assuming ChatSession has a QList<ChatMessage> messages;
        // Optionally, clear file transfer info if you want to remove those as well
        // it.value().fileTransfers.clear();
    }
    saveToFile(getChatHistoryFilePath());
}

// Clear chat history for all peers
void ChatManager::clearAllHistory() {
    for (auto it = chatSessions.begin(); it != chatSessions.end(); ++it) {
        it.value().messages.clear();
        // Optionally, clear file transfer info as well
        // it.value().fileTransfers.clear();
    }
    saveToFile(getChatHistoryFilePath());
}


void ChatManager::renderMessageToScrollArea(const QString& peerAddress, QScrollArea* scrollArea, const ChatMessage& message, int fontSize)
{
    // Ensure the session exists
    if (!chatSessions.contains(peerAddress)) {
        return;
    }

    // Get the chat widget (already created by tab logic)
    QWidget* chatWidget = scrollArea->widget();
    if (!chatWidget) {
        // Should not happen if tab creation logic is correct
        return;
    }

    // Get the layout (already created by tab logic)
    QVBoxLayout* layout = qobject_cast<QVBoxLayout*>(chatWidget->layout());
    if (!layout) {
        // Should not happen if tab creation logic is correct
        return;
    }

    // Remove stretch at the end if present
    int count = layout->count();
    if (count > 0) {
        QLayoutItem* lastItem = layout->itemAt(count - 1);
        if (lastItem && lastItem->spacerItem()) {
            QLayoutItem* stretch = layout->takeAt(count - 1);
            delete stretch;
        }
    }

    // Prepare sender display name
    const ChatSession& session = chatSessions[peerAddress];
    QString senderDisplayName = message.isFromMe ? "Me" : session.peerName;

    // Create and add the new message bubble
    MessageBubbleWidget* bubbleWidget = new MessageBubbleWidget(message, senderDisplayName);
    bubbleWidget->setFontSize(fontSize);
    layout->addWidget(bubbleWidget);

    // Add stretch to push messages to top
    layout->addStretch();

    // Scroll to bottom after a short delay
    QTimer::singleShot(10, [scrollArea]() {
        scrollArea->verticalScrollBar()->setValue(scrollArea->verticalScrollBar()->maximum());
    });
}



