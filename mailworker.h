#pragma once

#include <QObject>
#include <QString>
#include "mailitem.h"

class QProcess;

class MailWorker : public QObject
{
    Q_OBJECT
public:
    explicit MailWorker(QObject* parent = nullptr);
    ~MailWorker() override;
public slots:
    void start();
    void send(const MailItem& item);
    void startReceiver(int port, const QString& inboxDir);
    void stopReceiver();
signals:
    void mailSent(const MailItem& item);
    void mailFailed(const MailItem& item, const QString& error);
    void receiverStarted(bool ok, const QString& error);
    void receiverStopped();
private:
    QString buildMessage(const MailItem& item) const;
    QString forwarderPath() const;
    bool smtpSend(const MailItem& item, QString* error);
    void stopForwarder();

    QProcess* m_forwarder = nullptr;
    QProcess* m_receiver = nullptr;
};
