#include "torprocess.h"
#include<QThread>
#include<QSystemTrayIcon>
#include<QStyle>
#include<QApplication>
#include<QTcpSocket>
#include"constants.h"
#include"Notification.h"

QProcess* TorProcess::torProcess = nullptr;
TorProcess* TorProcess::torInstance = nullptr;


TorProcess::TorProcess() {
}

bool TorProcess::startTor(const QString& configPath) {
    QString torPath = findTorPath();
    if (torPath.isEmpty()) {
        Logger::log(Logger::ERROR, "Cannot start Tor - executable not found");
        return false;
    }

    torProcess = new QProcess();
    QStringList args;
    args << "-f" << configPath;

    Logger::log(Logger::INFO, "Starting Tor with config: " + configPath);
    torProcess->setProcessChannelMode(QProcess::MergedChannels);
    torProcess->start(torPath, args);

    if (torProcess->waitForStarted()) {
        Logger::log(Logger::INFO, "Tor process started successfully");
        QString output = torProcess->readAllStandardOutput();
        Logger::log(Logger::INFO, "Tor output: " + output);
        return true;
    }

    Logger::log(Logger::ERROR, "Failed to start Tor: " + torProcess->errorString());
    delete torProcess;
    torProcess = nullptr;
    return false;
}



void TorProcess::stopTor()
{
    if (torProcess) {
        torProcess->terminate();
        torProcess->waitForFinished(3000);
        if (torProcess->state() != QProcess::NotRunning) {
            torProcess->kill();
        }
        delete torProcess;
        torProcess = nullptr;
    }

    // Kill any lingering Tor processes
    QProcess cleanup;
    cleanup.start("killall", QStringList() << "-9" << "tor");
    cleanup.waitForFinished();

    Logger::log(Logger::INFO, "Tor processes cleaned up");
}

bool TorProcess::isTorRunning()
{

    return torProcess->state() == QProcess::Running;

}

QString TorProcess::findTorPath()
{
    QStringList possiblePaths = getPossibleTorPaths();

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
    paths << "Tor/tor.exe"
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

void TorProcess::cleanup() {
    // Make sure to stop Tor if it's running
    stopTor();
}

void TorProcess::killExistingTorProcesses() {
   Logger::log(Logger::INFO, "Checking for existing Tor processes...");

#ifdef Q_OS_WIN
    // On Windows, use taskkill which is generally available
    QProcess killProcess;
    killProcess.start("taskkill", QStringList() << "/F" << "/IM" << "tor.exe");
    killProcess.waitForFinished(3000);

    if (killProcess.exitCode() == 0) {
        Logger::log(Logger::INFO, "Existing Tor processes terminated");
    } else {
        Logger::log(Logger::INFO, "No existing Tor processes found or unable to terminate them");
    }
#else
    // On Unix-like systems, use a more portable approach
    // First try with killall (most Linux distributions)
    QProcess checkKillall;
    checkKillall.start("which", QStringList() << "killall");
    checkKillall.waitForFinished();

    if (checkKillall.exitCode() == 0) {
        // killall is available
        QProcess killProcess;
        killProcess.start("killall", QStringList() << "-9" << "tor");
        killProcess.waitForFinished(3000);
        Logger::log(Logger::INFO, "Attempted to terminate Tor processes using killall");
    } else {
        // Try with pkill (available on most Unix systems)
        QProcess checkPkill;
        checkPkill.start("which", QStringList() << "pkill");
        checkPkill.waitForFinished();

        if (checkPkill.exitCode() == 0) {
            // pkill is available
            QProcess killProcess;
            killProcess.start("pkill", QStringList() << "-9" << "-x" << "tor");
            killProcess.waitForFinished(3000);
            Logger::log(Logger::INFO, "Attempted to terminate Tor processes using pkill");
        } else {
             //Last resort: try to find and kill tor processes using ps and kill
            QProcess psProcess;
            psProcess.start("ps", QStringList() << "-e" << "-o" << "pid,comm");
            psProcess.waitForFinished();

            QString psOutput = psProcess.readAllStandardOutput();
            QStringList lines = psOutput.split("\n");

            for (const QString& line : lines) {
                if (line.contains(" tor")) {
                    QStringList parts = line.trimmed().split(" ", Qt::SkipEmptyParts);
                    if (!parts.isEmpty()) {
                        QString pid = parts.first();
                        QProcess killProcess;
                        killProcess.start("kill", QStringList() << "-9" << pid);
                        killProcess.waitForFinished();
                        Logger::log(Logger::INFO, "Killed Tor process with PID: " + pid);
                    }
                }
            }
        }
    }
#endif

    // Give the system a moment to fully release resources
    QThread::msleep(500);
}

void TorProcess::newTorCircuit() {
    if (!isTorRunning()) {
        Logger::log(Logger::ERROR, "Cannot request new circuit - Tor is not running");
        Notification("Tor Circuit", "Cannot request new circuit - Tor is not running", 3000,
                           QSystemTrayIcon::Warning);
        return;
    }

QTcpSocket socket;
Logger::log(Logger::INFO, "Attempting to connect to control port 9051...");
//socket.connectToHost("localhost", 9051);
socket.connectToHost("localhost", DefaultControlPort);

if (socket.waitForConnected(3000)) {
    Logger::log(Logger::INFO, "Connected to control port");
    socket.write("AUTHENTICATE\r\n");
    Logger::log(Logger::INFO, "Sent AUTHENTICATE");
    socket.write("SIGNAL NEWNYM\r\n");
    Logger::log(Logger::INFO, "Sent SIGNAL NEWNYM");
    socket.waitForBytesWritten();

    if (socket.waitForReadyRead(3000)) {
        QString response = socket.readAll();
        Logger::log(Logger::INFO, "Received response: " + response);
        socket.close();
        if (response.contains("250 OK")) {
            Logger::log(Logger::INFO, "New Tor circuit requested - path changed");

            Notification("Tor Circuit", "New Tor circuit established successfully");

            return;
        }
    } else {
        Logger::log(Logger::ERROR, "No response from control port: " + socket.errorString());
    }
    socket.close();
} else {
    Logger::log(Logger::ERROR, "Failed to connect to control port: " + socket.errorString());
}

Logger::log(Logger::ERROR, "Failed to request new circuit");

Notification("Tor Circuit", "Failed to request new circuit", 3000, QSystemTrayIcon::Critical);

}

