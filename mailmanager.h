#pragma once

#include <QObject>
#include <QList>
#include <QString>
#include "mailaccount.h"
#include"torconfig.h"
#include"mailitem.h"

class MailWorker;
class QProcess;

class MailManager : public QObject
{
    Q_OBJECT
public:
    explicit MailManager(TorConfig* torConfig, QObject* parent = nullptr);
    ~MailManager() override;

    static QString baseDir();
    static QString mailDir();
    static QString emailDir();
    static QString sentDir();
    static QString failedDir();
    static QString queueDir();
    static QString inboxDir();
    static QString accountsPath();

    QList<MailAccount> accounts() const { return m_accounts; }
    bool hasAccounts() const { return !m_accounts.isEmpty(); }
    QString activeAddress() const;
    void setActiveAccount(const QString& address);
    bool createAccount(const QString& username, QString* error);
    bool removeAccount(const QString& address, QString* error);

    QList<QJsonObject> sentMessages() const;
    QList<QJsonObject> failedMessages() const;
    QList<QJsonObject> queueMessages() const;
    QList<QJsonObject> inboxMessages() const;

    void send(const QString& to, const QString& subject, const QString& body,
              QString* error);

    void retryFailed(const QString& id);

    void setWorker(MailWorker* worker) { m_worker = worker; }

    void load();
    void save() const;
    bool isReceiverRunning() const { return m_receiverRunning; }

signals:
    void accountsChanged();
    void messagesChanged();
    void inboxChanged();
    void error(const QString& message);
    void mailDelivered(const QString& to, const QString& subject);
    void receiverRunningChanged(bool running);
    void receiverError(const QString& message);
    void messageReceived(int count);
public slots:
    void startReceiver(int port = 2525);
    void stopReceiver();
    void onMailSent(const MailItem& item);
    void onMailFailed(const MailItem& item, const QString& error);

private:
    void ensureFolders();

    QList<MailAccount> m_accounts;
    QString m_active;
    MailWorker* m_worker = nullptr;
    TorConfig* m_torConfig = nullptr;

public slots:
    void onReceiverStarted(bool ok, const QString& error);
    void onReceiverStopped();

private:
    bool m_receiverRunning = false;
    int m_lastInboxCount = 0;
};
