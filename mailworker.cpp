#include "mailworker.h"

#include <QProcess>
#include <QTcpSocket>
#include <QUuid>
#include <QDateTime>
#include<QElapsedTimer>
#include"logger.h"

MailWorker::MailWorker(QObject* parent) : QObject(parent) {}

MailWorker::~MailWorker()
{
    stopForwarder();

    if (m_receiver) {
        m_receiver->kill();
        m_receiver->waitForFinished(2000);
        delete m_receiver;
        m_receiver = nullptr;
    }
}

void MailWorker::start() {}

QString MailWorker::forwarderPath() const
{
#ifdef FLATHUB_BUILD
    return "/app/bin/onion-smtp-forwarder";
#else
    return "/usr/local/bin/onion-smtp-forwarder";
#endif
}


void MailWorker::stopForwarder()
{
    if (!m_forwarder) return;
    m_forwarder->kill();
    m_forwarder->waitForFinished(2000);
    delete m_forwarder;
    m_forwarder = nullptr;
}


QString MailWorker::buildMessage(const MailItem& item) const
{
    QString msg;
    msg += "From: " + item.from + "\r\n";
    msg += "To: " + item.to + "\r\n";
    msg += "Subject: " + QString(item.subject).replace("\r","").replace("\n"," ") + "\r\n";
    msg += "Date: " + QDateTime::currentDateTime().toString(Qt::RFC2822Date) + "\r\n";
    msg += "Message-ID: <" + item.id + "@" + item.from.section('@', 1) + ">\r\n";
    msg += "MIME-Version: 1.0\r\n";
    msg += "Content-Type: text/plain; charset=UTF-8\r\n";
    msg += "Content-Transfer-Encoding: 7bit\r\n";
    msg += "\r\n";

    QString body = item.body;
    body.replace("\r\n", "\n");
    body.replace("\n", "\r\n");
    body.replace("\r\n.", "\r\n..");
    if (body.startsWith('.'))
        body.prepend('.');
    msg += body;
    if (!msg.endsWith("\r\n"))
        msg += "\r\n";
    msg += ".\r\n";
    return msg;
}

/*
bool MailWorker::smtpSend(const MailItem& item, QString* error)
{
    const QString recipientOnion = item.to.section('@', 1);

    stopForwarder();

    m_forwarder = new QProcess(this);
    m_forwarder->setProcessChannelMode(QProcess::SeparateChannels);



    {
        QTcpSocket warm;
        warm.connectToHost("127.0.0.1", 9050);
        if (warm.waitForConnected(2000)) {
            warm.write(QByteArray("\x05\x01\x00", 3));
            warm.waitForBytesWritten(500);
            warm.waitForReadyRead(2000);
            warm.readAll();
            const QByteArray onion = recipientOnion.toUtf8();
            QByteArray req = QByteArray("\x05\x01\x00\x03", 4);
            req.append(char(onion.size()));
            req.append(onion);
            req.append(char(0)); req.append(char(25));
            warm.write(req);
            warm.waitForBytesWritten(500);
            warm.waitForReadyRead(30000);   // this is where the cold-circuit delay lives
            warm.disconnectFromHost();
        }
    }


    m_forwarder->start(forwarderPath(), { "23000", recipientOnion });

    if (!m_forwarder->waitForStarted(3000)) {

        *error = "Forwarder failed to start: " + m_forwarder->errorString();
        stopForwarder();
        return false;
    }

    bool bound = false;
    QElapsedTimer t; t.start();
    while (t.elapsed() < 5000) {
        const qint64 elapsed = t.elapsed();
        const bool ready = m_forwarder->waitForReadyRead(200);

        const QByteArray errChunk = m_forwarder->readAllStandardError();
        const QByteArray outChunk = m_forwarder->readAllStandardOutput();



        if (errChunk.contains("listening on") || outChunk.contains("listening on")) {
            bound = true;
            break;
        }
    }

    if (!bound) {
        m_forwarder->waitForFinished(1000);
        const QByteArray errOut = m_forwarder->readAllStandardError();
        const QByteArray stdOut = m_forwarder->readAllStandardOutput();

        *error = "Forwarder did not bind."
               + QString(" state=%1 exit=%2")
                     .arg(int(m_forwarder->state()))
                     .arg(m_forwarder->exitCode())
               + " stderr=[" + QString::fromUtf8(errOut).trimmed() + "]"
               + " stdout=[" + QString::fromUtf8(stdOut).trimmed() + "]";
        stopForwarder();
        return false;
    }

    QTcpSocket sock;
    sock.connectToHost("127.0.0.1", 23000);
    if (!sock.waitForConnected(5000)) {
        *error = "Cannot connect to forwarder: " + sock.errorString();
        stopForwarder();
        return false;
    }

    auto readReply = [&]() -> QString {
        QString full;
        while (true) {
            while (!sock.canReadLine()) {
                const bool r = sock.waitForReadyRead(30000);

                if (!r) return full;
            }
            const QString line = QString::fromUtf8(sock.readLine());
            full += line;
            if (line.size() < 4 || line[3] != '-') break;
        }
        return full;
    };

    auto cmd = [&](const QString& c) -> QString {
        sock.write(c.toUtf8() + "\r\n");
        if (!sock.waitForBytesWritten(10000)) {
            return {};
        }
        return readReply();
    };

    const QString greeting = readReply();
    if (!greeting.startsWith("220")) {
        *error = "Bad greeting: " + greeting.trimmed();
        stopForwarder();
        return false;
    }

    const QString ehlo = cmd("EHLO selene");
    if (!ehlo.startsWith("250")) {
        *error = "EHLO failed: " + ehlo.trimmed();
        stopForwarder();
        return false;
    }

    const QString mf = cmd("MAIL FROM:<" + item.from + ">");
    if (!mf.startsWith("250")) {
        *error = "MAIL FROM rejected: " + mf.trimmed();
        stopForwarder();
        return false;
    }

    const QString rc = cmd("RCPT TO:<" + item.to + ">");
    if (!rc.startsWith("250")) {
        *error = "RCPT TO rejected: " + rc.trimmed();
        stopForwarder();
        return false;
    }

    const QString dt = cmd("DATA");
    if (!dt.startsWith("354")) {
        *error = "DATA rejected: " + dt.trimmed();
        stopForwarder();
        return false;
    }

    const QString msg = buildMessage(item);
    sock.write(msg.toUtf8());
    if (!sock.waitForBytesWritten(60000)) {
        *error = "Body write failed: " + sock.errorString();
        stopForwarder();
        return false;
    }

    const QString final = readReply();

    sock.write("QUIT\r\n");
    sock.waitForBytesWritten(2000);
    sock.disconnectFromHost();

    stopForwarder();

    if (!final.startsWith("250")) {
        *error = "Message rejected: " + final.trimmed();
        return false;
    }
    return true;
}
*/

bool MailWorker::smtpSend(const MailItem& item, QString* error)
{
    const QString recipientOnion = item.to.section('@', 1);

    stopForwarder();

    m_forwarder = new QProcess(this);
    m_forwarder->setProcessChannelMode(QProcess::SeparateChannels);



    {
        QTcpSocket warm;
        warm.connectToHost("127.0.0.1", 9050);
        if (warm.waitForConnected(2000)) {
            warm.write(QByteArray("\x05\x01\x00", 3));
            warm.waitForBytesWritten(500);
            warm.waitForReadyRead(2000);
            warm.readAll();
            const QByteArray onion = recipientOnion.toUtf8();
            QByteArray req = QByteArray("\x05\x01\x00\x03", 4);
            req.append(char(onion.size()));
            req.append(onion);
            req.append(char(0)); req.append(char(25));
            warm.write(req);
            warm.waitForBytesWritten(500);
            warm.waitForReadyRead(30000);   // this is where the cold-circuit delay lives
            warm.disconnectFromHost();
        } else {
            Logger::log(Logger::WARNING, "SOCKS pre-warm failed: " + warm.errorString());
        }
    }


    m_forwarder->start(forwarderPath(), { "23000", recipientOnion });

    if (!m_forwarder->waitForStarted(3000)) {
        *error = "Forwarder failed to start: " + m_forwarder->errorString();
        Logger::log(Logger::ERROR, *error);
        stopForwarder();
        return false;
    }

    bool bound = false;
    QElapsedTimer t; t.start();
    while (t.elapsed() < 5000) {
        const qint64 elapsed = t.elapsed();
        const bool ready = m_forwarder->waitForReadyRead(200);

        const QByteArray errChunk = m_forwarder->readAllStandardError();
        const QByteArray outChunk = m_forwarder->readAllStandardOutput();



        if (errChunk.contains("listening on") || outChunk.contains("listening on")) {
            bound = true;
            break;
        }
    }

    if (!bound) {
        m_forwarder->waitForFinished(1000);
        const QByteArray errOut = m_forwarder->readAllStandardError();
        const QByteArray stdOut = m_forwarder->readAllStandardOutput();

        *error = "Forwarder did not bind."
               + QString(" state=%1 exit=%2")
                     .arg(int(m_forwarder->state()))
                     .arg(m_forwarder->exitCode())
               + " stderr=[" + QString::fromUtf8(errOut).trimmed() + "]"
               + " stdout=[" + QString::fromUtf8(stdOut).trimmed() + "]";
        Logger::log(Logger::ERROR, *error);
        stopForwarder();
        return false;
    }

    QTcpSocket sock;
    sock.connectToHost("127.0.0.1", 23000);
    if (!sock.waitForConnected(5000)) {
        *error = "Cannot connect to forwarder: " + sock.errorString();
        Logger::log(Logger::ERROR, *error);
        stopForwarder();
        return false;
    }

    auto readReply = [&]() -> QString {
        QString full;
        while (true) {
            while (!sock.canReadLine()) {
                const bool r = sock.waitForReadyRead(30000);

                if (!r) return full;
            }
            const QString line = QString::fromUtf8(sock.readLine());
            full += line;
            if (line.size() < 4 || line[3] != '-') break;
        }
        return full;
    };

    auto cmd = [&](const QString& c) -> QString {
        sock.write(c.toUtf8() + "\r\n");
        if (!sock.waitForBytesWritten(10000)) {
            return {};
        }
        return readReply();
    };

    const QString greeting = readReply();
    if (!greeting.startsWith("220")) {
        *error = "Bad greeting: " + greeting.trimmed();
        Logger::log(Logger::ERROR, *error);
        stopForwarder();
        return false;
    }

    const QString ehlo = cmd("EHLO selene");
    if (!ehlo.startsWith("250")) {
        *error = "EHLO failed: " + ehlo.trimmed();
        Logger::log(Logger::ERROR, *error);
        stopForwarder();
        return false;
    }

    const QString mf = cmd("MAIL FROM:<" + item.from + ">");
    if (!mf.startsWith("250")) {
        *error = "MAIL FROM rejected: " + mf.trimmed();
        Logger::log(Logger::ERROR, *error);
        stopForwarder();
        return false;
    }

    const QString rc = cmd("RCPT TO:<" + item.to + ">");
    if (!rc.startsWith("250")) {
        *error = "RCPT TO rejected: " + rc.trimmed();
        Logger::log(Logger::ERROR, *error);
        stopForwarder();
        return false;
    }

    const QString dt = cmd("DATA");
    if (!dt.startsWith("354")) {
        *error = "DATA rejected: " + dt.trimmed();
        Logger::log(Logger::ERROR, *error);
        stopForwarder();
        return false;
    }

    const QString msg = buildMessage(item);
    sock.write(msg.toUtf8());
    if (!sock.waitForBytesWritten(60000)) {
        *error = "Body write failed: " + sock.errorString();
        Logger::log(Logger::ERROR, *error);
        stopForwarder();
        return false;
    }

    const QString final = readReply();

    sock.write("QUIT\r\n");
    sock.waitForBytesWritten(2000);
    sock.disconnectFromHost();

    stopForwarder();

    if (!final.startsWith("250")) {
        *error = "Message rejected: " + final.trimmed();
        Logger::log(Logger::ERROR, *error);
        return false;
    }
    Logger::log(Logger::INFO, QString("Mail sent: %1 -> %2").arg(item.from, item.to));
    return true;
}

void MailWorker::send(const MailItem& item)
{
    QString err;
    if (smtpSend(item, &err))
        emit mailSent(item);
    else
        emit mailFailed(item, err);
}


void MailWorker::startReceiver(int port, const QString& inboxDir)
{
    stopReceiver();

    m_receiver = new QProcess(this);
    m_receiver->setProcessChannelMode(QProcess::MergedChannels);
#ifdef FLATHUB_BUILD
    const QString path = "/app/bin/onion-smtp-receiver";
#else
    const QString path = "/usr/local/bin/onion-smtp-receiver";
#endif
    m_receiver->start(path, { QString::number(port), inboxDir });

    if (!m_receiver->waitForStarted(3000)) {
        const QString err = m_receiver->errorString();
        delete m_receiver;
        m_receiver = nullptr;
        emit receiverStarted(false, err);
        return;
    }

    QElapsedTimer t; t.start();
    bool bound = false;
    while (t.elapsed() < 5000) {
        m_receiver->waitForReadyRead(200);
        const QByteArray out = m_receiver->readAllStandardOutput();
        if (out.contains("listening on")) { bound = true; break; }
    }

    if (!bound) {
        m_receiver->waitForFinished(500);
        const QByteArray out = m_receiver->readAllStandardOutput();
        const QString err = QString::fromUtf8(out).trimmed();
        stopReceiver();
        emit receiverStarted(false, err.isEmpty() ? "receiver did not bind" : err);
        return;
    }

    emit receiverStarted(true, QString());
}

void MailWorker::stopReceiver()
{
    if (!m_receiver) return;
    m_receiver->kill();
    m_receiver->waitForFinished(2000);
    delete m_receiver;
    m_receiver = nullptr;
    emit receiverStopped();
}
