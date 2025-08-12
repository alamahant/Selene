#ifndef CHATMANAGER_H
#define CHATMANAGER_H

#include <QObject>
#include <QMap>
#include <QDateTime>
#include <QString>
#include <QTextDocument>
#include <QTextBrowser>
#include"messagebubblewidget.h"
#include <QScrollArea>
#include <QVBoxLayout>

class ChatManager : public QObject
{
    Q_OBJECT

public:
    explicit ChatManager(QObject *parent = nullptr);

    // Session management
    void createSession(const QString& peerAddress, const QString& peerName);
    void activateSession(const QString& peerAddress);
    void closeSession(const QString& peerAddress);
    bool hasActiveSession() const;
    QString getActiveSessionPeer() const;
    QStringList getAllSessionPeers() const;

    // Message handling
    void addMessage(const QString& peerAddress, const QString& content, bool isFromMe);
    QList<ChatMessage> getMessages(const QString& peerAddress, int count = -1);
    void markAsRead(const QString& peerAddress);

    // UI rendering
    void renderMessagesToScrollArea(const QString& peerAddress, QScrollArea* scrollArea, int fontSize = 11);
    void renderMessageToScrollArea(const QString& peerAddress, QScrollArea* scrollArea, const ChatMessage& message, int fontSize= 11);


signals:
    void messageAdded(const QString& peerAddress, const ChatMessage& message);
    void sessionCreated(const QString& peerAddress);
    void sessionClosed(const QString& peerAddress);
    void sessionActivated(const QString& peerAddress);

private:
    QMap<QString, ChatSession> chatSessions;
    QString activeSessionPeer;

    // Helper methods
    QString formatMessageHTML(const ChatMessage& message, bool consecutive);
    QString getTimeString(const QDateTime& timestamp);
    QString getDateHeader(const QDateTime& timestamp, const QDate& prevDate);
    QString formatMessageTime(const QDateTime& timestamp);
public slots:
    // In ChatManager.h (add to public section)
    bool saveToFile(const QString& filePath) const;
    bool loadFromFile(const QString& filePath);
// Filetransfers
public:
    // Add a file message to the chat (sent or received)
    void addFileMessage(const QString& peerAddress, const QString& fileName, qint64 fileSize, bool isFromMe);

    // Update progress for an ongoing file transfer
    void updateFileTransferProgress(const QString& peerAddress, const QString& fileName, qint64 bytesTransferred, qint64 totalBytes);

    // Mark a file transfer as complete
    void markFileTransferComplete(const QString& peerAddress, const QString& fileName);

    // Mark a file transfer as failed
    void markFileTransferFailed(const QString& peerAddress, const QString& fileName, const QString& error);

    QMap<QString, ChatSession>& getChatSessions();

signals:
    // Emitted when a file message is added (optional, for UI updates)
    void fileMessageAdded(const QString& peerAddress, const ChatMessage& message);
private:

    ChatMessage* findFileMessage(const QString& peerAddress, const QString& fileName);
public:
    void clearHistoryForPeer(const QString& peerOnion);
    void clearAllHistory();
    int fontSize = 11;
};

#endif // CHATMANAGER_H

