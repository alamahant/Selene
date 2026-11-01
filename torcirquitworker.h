#pragma once

#include <QObject>
#include <QStringList>

class TorCircuitWorker : public QObject
{
    Q_OBJECT
public:
    explicit TorCircuitWorker(QObject* parent = nullptr);

public slots:
    void fetch(int controlPort, int socksPort);

signals:
    void done(const QStringList& countries);
    void failed(const QString& reason);

private:
    QStringList queryCircuits(class QTcpSocket& ctrl);
    QString lookupCountry(class QTcpSocket& ctrl, const QString& fingerprint);
    static QString parseCountry(const QString& response);   // <-- add this
};
