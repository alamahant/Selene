#include "torcirquitworker.h"
#include <QTcpSocket>
#include <QNetworkProxy>

TorCircuitWorker::TorCircuitWorker(QObject* parent)
    : QObject(parent)
{
}

void TorCircuitWorker::fetch(int controlPort, int socksPort)
{
    {
        QTcpSocket probe;
        QNetworkProxy socks(QNetworkProxy::Socks5Proxy, "127.0.0.1", socksPort);
        probe.setProxy(socks);
        probe.connectToHost("127.0.0.1", 1);   // fast local target
        probe.waitForConnected(3000);
        probe.abort();
    }

    QTcpSocket ctrl;
    ctrl.connectToHost("127.0.0.1", controlPort);
    if (!ctrl.waitForConnected(3000)) {
        emit failed("control port not reachable");
        return;
    }

    ctrl.write("AUTHENTICATE\r\n");
    ctrl.write("GETINFO circuit-status\r\n");
    ctrl.waitForBytesWritten(500);
    if (!ctrl.waitForReadyRead(2000)) {
        emit failed("no response from control port");
        return;
    }

    const QString resp = QString::fromUtf8(ctrl.readAll());

    QString path;
    for (const QString& line : resp.split('\n')) {
        if (!line.contains("BUILT"))
            continue;
        const QStringList parts = line.split(' ', Qt::SkipEmptyParts);
        if (parts.size() < 3)
            continue;
        const QStringList relays = parts[2].split(',');
        if (relays.size() == 3) {
            path = parts[2];
            break;
        }
    }
    if (path.isEmpty()) {
        emit failed("no built 3-hop circuit");
        return;
    }

    QStringList countries;
    for (const QString& relay : path.split(',')) {
        QString fp = relay;
        if (fp.startsWith('$'))
            fp.remove(0, 1);
        const int tilde = fp.indexOf('~');
        if (tilde >= 0)
            fp = fp.left(tilde);
        countries << lookupCountry(ctrl, fp);
    }
    ctrl.close();
    emit done(countries);
}

QString TorCircuitWorker::lookupCountry(QTcpSocket& ctrl, const QString& fingerprint)
{
    ctrl.write(QString("GETINFO ns/id/%1\r\n").arg(fingerprint).toUtf8());
    ctrl.waitForBytesWritten(500);
    if (!ctrl.waitForReadyRead(2000))
        return "??";
    const QString ns = QString::fromUtf8(ctrl.readAll());

    QString ip;
    for (const QString& line : ns.split('\n')) {
        if (!line.startsWith("r "))
            continue;
        const QStringList parts = line.split(' ', Qt::SkipEmptyParts);
        if (parts.size() >= 7) {
            ip = parts[6];
            break;
        }
    }
    if (ip.isEmpty())
        return "??";

    ctrl.write(QString("GETINFO ip-to-country/%1\r\n").arg(ip).toUtf8());
    ctrl.waitForBytesWritten(500);
    if (!ctrl.waitForReadyRead(2000))
        return "??";
    const QString cr = QString::fromUtf8(ctrl.readAll());

    return parseCountry(cr);
}

QString TorCircuitWorker::parseCountry(const QString& response)
{
    const int eq = response.indexOf('=');
    if (eq < 0)
        return "??";

    int start = eq + 1;
    int end = start;
    while (end < response.size()
           && response[end] != '\r'
           && response[end] != '\n'
           && response[end] != '"'
           && response[end] != ',')
        ++end;

    const QString code = response.mid(start, end - start).trimmed();
    return code.isEmpty() ? QString("??") : code.toUpper();
}
