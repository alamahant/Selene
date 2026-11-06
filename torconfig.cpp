#include "torconfig.h"
#include "logger.h"
#include <QDir>
#include <QTextStream>
#include"constants.h"

TorConfig::TorConfig()
{
    QString appDir = getAppDataDir();

    hiddenServiceDir = appDir + "/.selene/hidden_service";
    fileHiddenServiceDir = appDir + "/.selene/file_hidden_service";
    mailHiddenServiceDir = appDir + "/.selene/mail_hidden_service";
    torrcPath = appDir + "/.selene/torrc";
    wwwPath = appDir + "/www";
    QDir().mkpath(wwwPath);
    if (QDir(wwwPath).exists()) {
        if (!QFile(wwwPath).setPermissions(QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner)) {
            Logger::log(Logger::WARNING, "Failed to set permissions for www directory: " + wwwPath);
        }
    }
}

/*
bool TorConfig::setupHiddenService()
{
    QDir dir(hiddenServiceDir);
    QFile torrcFile(torrcPath);

    if (dir.exists() && torrcFile.exists()) {
        QFile dirFile(hiddenServiceDir);
        if (dirFile.permissions() == (QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner) &&
            torrcFile.permissions() == (QFile::ReadOwner | QFile::WriteOwner)) {
            Logger::log(Logger::INFO, "Using existing configuration");
            return true;
        }
    }

    if (!dir.exists()) {
        if (!QDir().mkpath(hiddenServiceDir)) {
            return false;
        }
        QFile(hiddenServiceDir).setPermissions(QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner);
    }

    if (!torrcFile.exists()) {
        if (!createTorrcEntry()) {
            return false;
        }
        torrcFile.setPermissions(QFile::ReadOwner | QFile::WriteOwner);
    }

    return true;
}
*/

bool TorConfig::setupHiddenService()
{
    QDir dir(hiddenServiceDir);
    QDir fileDir(fileHiddenServiceDir);
    QDir mailDir(mailHiddenServiceDir);
    QFile torrcFile(torrcPath);

    if (dir.exists() && fileDir.exists() && mailDir.exists() && torrcFile.exists()) {
        QFile dirFile(hiddenServiceDir);
        if (dirFile.permissions() == (QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner) &&
            torrcFile.permissions() == (QFile::ReadOwner | QFile::WriteOwner)) {
            Logger::log(Logger::INFO, "Using existing configuration");
            return true;
        }
    }

    if (!dir.exists()) {
        if (!QDir().mkpath(hiddenServiceDir)) return false;
        QFile(hiddenServiceDir).setPermissions(QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner);
    }

    if (!fileDir.exists()) {
        if (!QDir().mkpath(fileHiddenServiceDir)) return false;
        QFile(fileHiddenServiceDir).setPermissions(QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner);
    }

    if (!mailDir.exists()) {
        if (!QDir().mkpath(mailHiddenServiceDir)) return false;
        QFile(mailHiddenServiceDir).setPermissions(QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner);
    }

    if (!torrcFile.exists()) {
        if (!createTorrcEntry()) return false;
        torrcFile.setPermissions(QFile::ReadOwner | QFile::WriteOwner);
    }

    return true;
}

/*
bool TorConfig::createTorrcEntry()
{
    QFile torrc(torrcPath);
    if (!torrc.open(QIODevice::WriteOnly | QIODevice::Text)) {
        Logger::log(Logger::ERROR, "Cannot open torrc for writing");
        return false;
    }

    QTextStream out(&torrc);
    out << "HiddenServiceDir " << hiddenServiceDir << "\n";
    out << "HiddenServicePort 80 127.0.0.1:8080\n";
    out << "HiddenServiceDir " << fileHiddenServiceDir << "\n";
    out << "HiddenServicePort 80 127.0.0.1:9090\n";
    out << "ControlPort 9051\n";
    out << "CookieAuthentication 0\n";
    out << "SocksPort 9050\n";

    Logger::log(Logger::INFO, "Torrc file created successfully");
    return true;
}
*/

bool TorConfig::createTorrcEntry()
{
    QFile torrc(torrcPath);
    if (!torrc.open(QIODevice::WriteOnly | QIODevice::Text)) {
        Logger::log(Logger::ERROR, "Cannot open torrc for writing");
        return false;
    }

    QTextStream out(&torrc);
    out << "HiddenServiceDir " << hiddenServiceDir << "\n";
    out << "HiddenServicePort 80 127.0.0.1:8080\n";
    out << "HiddenServiceDir " << fileHiddenServiceDir << "\n";
    out << "HiddenServicePort 80 127.0.0.1:9090\n";
    out << "HiddenServiceDir " << mailHiddenServiceDir << "\n";
    out << "HiddenServicePort 25 127.0.0.1:2525\n";
    out << "ControlPort 9051\n";
    out << "CookieAuthentication 0\n";
    out << "SocksPort 9050\n";

    Logger::log(Logger::INFO, "Torrc file created successfully");
    return true;
}

/*
bool TorConfig::readOnionAddress()
{
    bool ok = true;
    for(int i = 0; i < 5; i++) {
        QFile hostname(hiddenServiceDir + "/hostname");
        if (hostname.exists() && hostname.open(QIODevice::ReadOnly | QIODevice::Text)) {
            onionAddress = QString(hostname.readAll()).trimmed();
            Logger::log(Logger::INFO, "Onion address loaded: " + onionAddress);
            break;
        }
        QThread::sleep(1);

        if (onionAddress.isEmpty()) {
            Logger::log(Logger::ERROR, "Failed to read chat hostname file after 5 attempts");
            ok = false;
        }

    }

    for (int i = 0; i < 5; i++) {
        QFile hostname(fileHiddenServiceDir + "/hostname");
        if (hostname.exists() && hostname.open(QIODevice::ReadOnly | QIODevice::Text)) {
            fileOnionAddress = QString(hostname.readAll()).trimmed();
            Logger::log(Logger::INFO, "File onion address loaded: " + fileOnionAddress);
            break;
        }
        QThread::sleep(1);
    }
    if (fileOnionAddress.isEmpty()) {
        Logger::log(Logger::ERROR, "Failed to read file hostname file after 5 attempts");
        ok = false;
    }

    return ok;
}
*/

bool TorConfig::readOnionAddress()
{
    bool ok = true;

    for (int i = 0; i < 5; i++) {
        QFile hostname(hiddenServiceDir + "/hostname");
        if (hostname.exists() && hostname.open(QIODevice::ReadOnly | QIODevice::Text)) {
            onionAddress = QString(hostname.readAll()).trimmed();
            Logger::log(Logger::INFO, "Onion address loaded: " + onionAddress);
            break;
        }
        QThread::sleep(1);
    }
    if (onionAddress.isEmpty()) {
        Logger::log(Logger::ERROR, "Failed to read chat hostname file after 5 attempts");
        ok = false;
    }

    for (int i = 0; i < 5; i++) {
        QFile hostname(fileHiddenServiceDir + "/hostname");
        if (hostname.exists() && hostname.open(QIODevice::ReadOnly | QIODevice::Text)) {
            fileOnionAddress = QString(hostname.readAll()).trimmed();
            Logger::log(Logger::INFO, "File onion address loaded: " + fileOnionAddress);
            break;
        }
        QThread::sleep(1);
    }
    if (fileOnionAddress.isEmpty()) {
        Logger::log(Logger::ERROR, "Failed to read file hostname file after 5 attempts");
        ok = false;
    }

    for (int i = 0; i < 5; i++) {
        QFile hostname(mailHiddenServiceDir + "/hostname");
        if (hostname.exists() && hostname.open(QIODevice::ReadOnly | QIODevice::Text)) {
            mailOnionAddress = QString(hostname.readAll()).trimmed();
            Logger::log(Logger::INFO, "Mail onion address loaded: " + mailOnionAddress);
            break;
        }
        QThread::sleep(1);
    }
    if (mailOnionAddress.isEmpty()) {
        Logger::log(Logger::ERROR, "Failed to read mail hostname file after 5 attempts");
        ok = false;
    }

    return ok;
}

QString TorConfig::getFileOnionAddress() const
{
    return fileOnionAddress;
}

QString TorConfig::getTorrcPath() const
{
    return torrcPath;
}

QString TorConfig::getMailHiddenServiceDir() const
{
    return mailHiddenServiceDir;
}


QString TorConfig::getMailOnionAddress() const
{
    return mailOnionAddress;
}
