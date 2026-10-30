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

    void createSession(const QString& peerAddress, const QString& peerName);
    void activateSession(const QString& peerAddress);
    void closeSession(const QString& peerAddress);
    bool hasActiveSession() const;
    QString getActiveSessionPeer() const;
    QStringList getAllSessionPeers() const;

    void addMessage(const QString& peerAddress, const QString& content, bool isFromMe);
    QList<ChatMessage> getMessages(const QString& peerAddress, int count = -1);
    void markAsRead(const QString& peerAddress);

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

    QString formatMessageHTML(const ChatMessage& message, bool consecutive);
    QString getTimeString(const QDateTime& timestamp);
    QString getDateHeader(const QDateTime& timestamp, const QDate& prevDate);
    QString formatMessageTime(const QDateTime& timestamp);
public slots:
    bool saveToFile(const QString& filePath) const;
    bool loadFromFile(const QString& filePath);
public:
    void addFileMessage(const QString& peerAddress, const QString& fileName, qint64 fileSize, bool isFromMe);

    void updateFileTransferProgress(const QString& peerAddress, const QString& fileName, qint64 bytesTransferred, qint64 totalBytes);

    void markFileTransferComplete(const QString& peerAddress, const QString& fileName);

    void markFileTransferFailed(const QString& peerAddress, const QString& fileName, const QString& error);

    QMap<QString, ChatSession>& getChatSessions();

signals:
    void fileMessageAdded(const QString& peerAddress, const ChatMessage& message);
private:

    ChatMessage* findFileMessage(const QString& peerAddress, const QString& fileName);
public:
    void clearHistoryForPeer(const QString& peerOnion);
    void clearAllHistory();
    int fontSize = 11;
};

#endif // CHATMANAGER_H

