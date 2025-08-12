#include "mainwindow.h"
#include<QObject>
#include <QApplication>
#include"torprocess.h"
#include <QLocalSocket>
#include <QLocalServer>
#include <QMessageBox>
#include "mainwindow.h"
#include"torprocess.h"
#include<QDir>
#include"constants.h"
#include<QDebug>
#include<QDirIterator>
#include"logger.h"
#include<QSoundEffect>
#include<QSettings>



int main(int argc, char *argv[])
{

#ifndef FLATHUB_BUILD
    QCoreApplication::setOrganizationName("Selene");
#endif
    QCoreApplication::setApplicationName("Selene");
    QCoreApplication::setApplicationVersion("1.0.0");

    Logger::init();

    //TorProcess::killExistingTorProcesses();
    QString appDataDir = getAppDataDir();

    QDir().mkpath(appDataDir); // Ensure the directory exists
    QDir().mkpath(getCryptoDirPath()); // Ensure the directory exists
    QDir().mkpath(getContactsDirPath()); // Ensure the directory exists
    QDir().mkpath(getFileSaveDirPath());
    QDir().mkpath(getConfigDirPath());
    QDir().mkpath(getDocumentsDirPath());
    QDir().mkpath(getWWWDir());

    QApplication a(argc, argv);
    //a.setQuitOnLastWindowClosed(true);

    // Check for existing instance
    const QString serverName = "Selene-SingleInstanceServer";
    QLocalSocket socket;
    socket.connectToServer(serverName);

    // If connection succeeds, another instance is already running
    if (socket.waitForConnected(500)) {
        QMessageBox::critical(nullptr, "Application Already Running",
                              "Another instance of P2P Chat is already running.\n"
                              "Only one instance of the application is allowed.");
        return 1;
    }

    // No existing instance found, start a new one
    QLocalServer server;
    // Make sure no previous server instance exists
    QLocalServer::removeServer(serverName);
    // Start the server
    if (!server.listen(serverName)) {
        QMessageBox::critical(nullptr, "Error",
                              "Could not create server for single instance check: " +
                                  server.errorString());
        return 1;
    }

    MainWindow w;
    w.show();
    QObject::connect(&a, &QApplication::aboutToQuit, []() {
        TorProcess::cleanup();
    });
    return a.exec();
}
