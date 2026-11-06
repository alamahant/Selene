#include "torprocess.h"
#include "constants.h"
#include "Notification.h"
#include "logger.h"

#include <QThread>
#include <QSystemTrayIcon>
#include <QApplication>
#include <QTcpSocket>
#include <QTimer>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QMessageBox>

TorProcess::TorProcess(QObject* parent)
    : QObject(parent)
{
}

TorProcess::~TorProcess()
{
    stopTor();
}

bool TorProcess::startTor(const QString& configPath)
{
    QString torPath = findTorPath();
    if (torPath.isEmpty()) {
        Logger::log(Logger::ERROR, "Cannot start Tor - executable not found");
        return false;
    }

    m_torProcess = new QProcess(this);
    QStringList args;
    args << "-f" << configPath;

    Logger::log(Logger::INFO, "Starting Tor with config: " + configPath);
    m_torProcess->setProcessChannelMode(QProcess::MergedChannels);
    m_torProcess->start(torPath, args);

    if (!m_torProcess->waitForStarted()) {
        Logger::log(Logger::ERROR, "Failed to start Tor: " + m_torProcess->errorString());
        delete m_torProcess;
        m_torProcess = nullptr;
        return false;
    }

    Logger::log(Logger::INFO, "Tor process started successfully");
    QString output = m_torProcess->readAllStandardOutput();
    Logger::log(Logger::INFO, "Tor output: " + output);

    const qint64 pid = m_torProcess->processId();
    if (pid > 0) {
        const QString pidPath = getAppDataDir() + "/torpid";
        QFile f(pidPath);
        if (f.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
            QTextStream out(&f);
            out << pid << '\n';
            f.close();
            Logger::log(Logger::INFO,
                QString("Tor PID %1 written to %2").arg(pid).arg(pidPath));
        } else {
            Logger::log(Logger::WARNING,
                "Could not write tor PID file: " + pidPath + " (" + f.errorString() + ")");
        }
    } else {
        Logger::log(Logger::WARNING, "Tor started but processId() returned 0");
    }

    trackBootstrapProgress();
    return true;
}

void TorProcess::stopTor()
{
    if (m_torProcess) {
        m_torProcess->terminate();
        if (!m_torProcess->waitForFinished(3000)) {
            m_torProcess->kill();
            m_torProcess->waitForFinished(1000);
        }
        delete m_torProcess;
        m_torProcess = nullptr;
    }

    killStoredTorPid();
    Logger::log(Logger::INFO, "Tor processes cleaned up");
}

bool TorProcess::isTorRunning() const
{
    return m_torProcess && m_torProcess->state() == QProcess::Running;
}

QString TorProcess::findTorPath()
{
    const QStringList possiblePaths = getPossibleTorPaths();
    for (const QString& path : possiblePaths) {
        if (QFileInfo(path).exists() && QFileInfo(path).isExecutable()) {
            Logger::log(Logger::INFO, "Found Tor at: " + path);
            return path;
        }
    }

    QString installMessage;
#ifdef Q_OS_WIN
    installMessage = "Tor is not installed. Please download and install Tor from:\n"
                     "https://www.torproject.org/download/";
#else
    installMessage = "Tor is not installed. Please install using your package manager:\n\n"
                     "Debian/Ubuntu: sudo apt install tor\n"
                     "Fedora: sudo dnf install tor\n"
                     "Arch: sudo pacman -S tor\n"
                     "Gentoo: sudo emerge -av tor";
#endif

    QMessageBox::critical(nullptr, "Tor Not Found", installMessage);
    Logger::log(Logger::ERROR, "Tor executable not found!");
    return QString();
}

QStringList TorProcess::getPossibleTorPaths()
{
    QStringList paths;
#ifdef Q_OS_WIN
    paths << QApplication::applicationDirPath() + "/tor.exe"
          << "Tor/tor.exe"
          << "C:/Program Files/Tor/tor.exe"
          << "C:/Program Files (x86)/Tor/tor.exe";
#else
    paths << "/app/bin/tor"
          << "/usr/bin/tor"
          << "/usr/sbin/tor"
          << "/usr/local/bin/tor";
#endif
    return paths;
}

void TorProcess::cleanup()
{
    stopTor();
}

void TorProcess::killExistingTorProcesses()
{
    Logger::log(Logger::INFO, "Checking for existing Tor processes...");

#ifdef Q_OS_WIN
    QProcess killProcess;
    killProcess.start("taskkill", QStringList() << "/F" << "/IM" << "tor.exe");
    killProcess.waitForFinished(3000);
    if (killProcess.exitCode() == 0)
        Logger::log(Logger::INFO, "Existing Tor processes terminated");
    else
        Logger::log(Logger::INFO, "No existing Tor processes found or unable to terminate them");
#else
    QProcess checkKillall;
    checkKillall.start("which", QStringList() << "killall");
    checkKillall.waitForFinished();
    if (checkKillall.exitCode() == 0) {
        QProcess killProcess;
        killProcess.start("killall", QStringList() << "-9" << "tor");
        killProcess.waitForFinished(3000);
        Logger::log(Logger::INFO, "Attempted to terminate Tor processes using killall");
    } else {
        QProcess checkPkill;
        checkPkill.start("which", QStringList() << "pkill");
        checkPkill.waitForFinished();
        if (checkPkill.exitCode() == 0) {
            QProcess killProcess;
            killProcess.start("pkill", QStringList() << "-9" << "-x" << "tor");
            killProcess.waitForFinished(3000);
            Logger::log(Logger::INFO, "Attempted to terminate Tor processes using pkill");
        } else {
            QProcess psProcess;
            psProcess.start("ps", QStringList() << "-e" << "-o" << "pid,comm");
            psProcess.waitForFinished();
            const QString psOutput = psProcess.readAllStandardOutput();
            for (const QString& line : psOutput.split("\n")) {
                if (line.contains(" tor")) {
                    const QStringList parts = line.trimmed().split(" ", Qt::SkipEmptyParts);
                    if (!parts.isEmpty()) {
                        QProcess killProcess;
                        killProcess.start("kill", QStringList() << "-9" << parts.first());
                        killProcess.waitForFinished();
                        Logger::log(Logger::INFO, "Killed Tor process with PID: " + parts.first());
                    }
                }
            }
        }
    }
#endif
    QThread::msleep(500);
}


void TorProcess::newTorCircuit()
{
    if (!isTorRunning()) {
        Logger::log(Logger::ERROR, "Cannot request new circuit - Tor is not running");
        Notification("Tor Circuit", "Cannot request new circuit - Tor is not running", 3000,
                     QSystemTrayIcon::Warning);
        return;
    }

    /*
    if (m_networkManager && !m_networkManager->getConnectedPeers().isEmpty()) {
        Logger::log(Logger::INFO, "New circuit blocked - a peer is connected");
        Notification("Tor Circuit",
                     "Disconnect from the peer before requesting a new circuit",
                     3000, QSystemTrayIcon::Warning);
        return;
    }
    */


    const bool idle = m_networkManager->getConnectedPeers().isEmpty();

    QTcpSocket socket;
    Logger::log(Logger::INFO, "Attempting to connect to control port 9051...");
    socket.connectToHost("localhost", DefaultControlPort);

    if (!socket.waitForConnected(3000)) {
        Logger::log(Logger::ERROR, "Failed to connect to control port: " + socket.errorString());
        Notification("Tor Circuit", "Failed to request new circuit", 3000,
                     QSystemTrayIcon::Critical);
        return;
    }

    Logger::log(Logger::INFO, "Connected to control port");
    socket.write("AUTHENTICATE\r\n");
    socket.write("SIGNAL NEWNYM\r\n");
    socket.waitForBytesWritten();
    socket.waitForReadyRead(3000);

    const QString response = QString::fromUtf8(socket.readAll());
    Logger::log(Logger::INFO, "Received response: " + response);

    if (!response.contains("250 OK")) {
        Logger::log(Logger::ERROR, "NEWNYM failed");
        socket.close();
        Notification("Tor Circuit", "Failed to request new circuit", 3000,
                     QSystemTrayIcon::Critical);
        return;
    }

    if (idle) {
        socket.write("GETINFO circuit-status\r\n");
        socket.waitForBytesWritten(500);
        socket.waitForReadyRead(2000);
        const QString circ = QString::fromUtf8(socket.readAll());

        QStringList idsToClose;
        for (const QString& line : circ.split('\n')) {
            if (!line.contains("BUILT") && !line.contains("LAUNCHED"))
                continue;
            const QStringList parts = line.split(' ', Qt::SkipEmptyParts);
            if (!parts.isEmpty())
                idsToClose << parts.first();
        }

        for (const QString& id : idsToClose) {
            socket.write(QString("CLOSECIRCUIT %1 IfUnused=false\r\n").arg(id).toUtf8());
        }
        socket.waitForBytesWritten(500);

        Logger::log(Logger::INFO, QString("Forced circuit change, closed %1 circuits")
                                       .arg(idsToClose.size()));
        Notification("Tor Circuit", "New Tor circuit established successfully");
    } else {
        Logger::log(Logger::INFO, "Peer connected - marking circuits dirty only (NEWNYM)");
        Notification("Tor Circuit",
                     "New circuit will be used for the next connection "
                     "(active sessions are not interrupted)");
    }

    socket.close();
}

void TorProcess::killStoredTorPid()
{
    const QString pidPath = getAppDataDir() + "/torpid";
    QFile f(pidPath);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        Logger::log(Logger::WARNING, QString("[tor-cleanup] No torpid file at %1").arg(pidPath));
        return;
    }

    bool ok = false;
    const qint64 pid = QString::fromUtf8(f.readAll()).trimmed().toLongLong(&ok);
    f.close();

    if (!ok || pid <= 0) {
        Logger::log(Logger::WARNING, "[tor-cleanup] torpid file empty or invalid.");
        QFile::remove(pidPath);
        return;
    }

#ifdef Q_OS_WIN
    QProcess p;
    p.start("taskkill", {"/F", "/PID", QString::number(pid)});
    p.waitForFinished(3000);
#else
    QProcess p;
    p.start("kill", {"-9", QString::number(pid)});
    p.waitForFinished(3000);
    Logger::log(Logger::INFO, QString("[tor-cleanup] kill -9 PID%1").arg(pid));
#endif

    QFile::remove(pidPath);
}

void TorProcess::trackBootstrapProgress()
{
    auto* poll = new QTimer(this);
    connect(poll, &QTimer::timeout, this, [this, poll]() {
        QTcpSocket sock;
        sock.connectToHost("127.0.0.1", DefaultControlPort);
        if (!sock.waitForConnected(500))
            return;   // control port not up yet, try next tick

        sock.write("AUTHENTICATE\r\n");
        sock.write("GETINFO status/bootstrap-phase\r\n");
        sock.waitForBytesWritten(500);
        sock.waitForReadyRead(1000);

        const QString resp = QString::fromUtf8(sock.readAll());
        sock.close();

        const int pct = parseProgress(resp);
        if (pct >= 0) {
            emit bootstrapProgress(pct);
            if (pct >= 100) {
                poll->stop();
                poll->deleteLater();
                emit torReady();
            }
        }
    });
    poll->start(1000);
}

int TorProcess::parseProgress(const QString& response)
{
    const QString marker = "PROGRESS=";
    int idx = response.indexOf(marker);
    if (idx < 0)
        return -1;

    idx += marker.size();
    int end = idx;
    while (end < response.size() && response[end].isDigit())
        ++end;

    if (end == idx)
        return -1;

    bool ok = false;
    const int value = response.mid(idx, end - idx).toInt(&ok);
    return ok ? value : -1;
}

void TorProcess::setNetworkManager(NetworkManager* nm)
{
    m_networkManager = nm;
}
