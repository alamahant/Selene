#ifndef TORCONFIG_H
#define TORCONFIG_H

#include <QString>
#include <QFile>
#include<QDir>
#include <QCoreApplication>
#include<QThread>
#include "logger.h"
class TorConfig
{
public:
    TorConfig();
    bool setupHiddenService();
    QString getOnionAddress() const { return onionAddress; }
    bool readOnionAddress();


public:     // Public data members (if any)
    static const int TOR_PORT = 9050;
    QString getFileOnionAddress() const;

private:
    QString hiddenServiceDir;
    QString filehiddenServiceDir;

    QString fileHiddenServiceDir;
    QString torrcPath;
    QString onionAddress;
    QString fileOnionAddress;
    QString wwwPath;
private:
    bool createTorrcEntry();
};

#endif // TORCONFIG_H
