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
#include<QStyleFactory>
#include<QSettings>
#include<QFont>
#include <QProcess>



static void killStoredTorPid()
{

    const QString pidPath = getAppDataDir() + "/torpid";
    QFile f(pidPath);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return;
    }

    bool ok = false;
    const qint64 pid = QString::fromUtf8(f.readAll()).trimmed().toLongLong(&ok);
    f.close();

    if (!ok || pid <= 0) {
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
#endif

    QFile::remove(pidPath);
}



int main(int argc, char *argv[])
{

    //#ifndef FLATHUB_BUILD
        QCoreApplication::setOrganizationName("Alamahant");
    //#endif
        QCoreApplication::setApplicationName("Selene");
        QCoreApplication::setApplicationVersion("1.1.1");

    killStoredTorPid();

    Logger::init();

    QString appDataDir = getAppDataDir();

    QDir().mkpath(appDataDir); // Ensure the directory exists
    QDir().mkpath(getCryptoDirPath()); // Ensure the directory exists
    QDir().mkpath(getContactsDirPath()); // Ensure the directory exists
    QDir().mkpath(getFileSaveDirPath());
    //QDir().mkpath(getConfigDirPath());
    QDir().mkpath(getDocumentsDirPath());
    QDir().mkpath(getWWWDir());
    QDir().mkpath(getDownloadsDirPath());
    QDir().mkpath(getWebsitesDir());

#ifdef Q_OS_WIN

    QSettings::setDefaultFormat(QSettings::IniFormat);
#endif

    QSettings s;
        double factor = s.value("ui/scaleFactor", 1.0).toDouble();
        qputenv("QT_SCALE_FACTOR", QByteArray::number(factor));
        FONTSIZE = s.value("ui/fontSize", DEFAULTFONTSIZE).toReal();

    QApplication a(argc, argv);


//#ifdef Q_OS_WIN
    a.setStyle(QStyleFactory::create("Fusion"));

    QPalette lightPalette;
    lightPalette.setColor(QPalette::Window, Qt::white);
    lightPalette.setColor(QPalette::WindowText, Qt::black);
    lightPalette.setColor(QPalette::Base, Qt::white);
    lightPalette.setColor(QPalette::Text, Qt::black);
    lightPalette.setColor(QPalette::Button, QColor(240, 240, 240));
    lightPalette.setColor(QPalette::ButtonText, Qt::black);
    lightPalette.setColor(QPalette::Highlight, QColor(0, 120, 215));
    lightPalette.setColor(QPalette::HighlightedText, Qt::white);

    a.setPalette(lightPalette);
//#endif


    if (FONTSIZE > 0.0) {
        QFont appFont = a.font();
        appFont.setPointSizeF(FONTSIZE);
        a.setFont(appFont);
    } else {
        FONTSIZE = DEFAULTFONTSIZE;
    }

    const QString serverName = "Selene-SingleInstanceServer";
    QLocalSocket socket;
    socket.connectToServer(serverName);

    if (socket.waitForConnected(500)) {
        QMessageBox::critical(nullptr, "Application Already Running",
                              "Another instance of P2P Chat is already running.\n"
                              "Only one instance of the application is allowed.");
        return 1;
    }

    QLocalServer server;
    QLocalServer::removeServer(serverName);
    if (!server.listen(serverName)) {
        QMessageBox::critical(nullptr, "Error",
                              "Could not create server for single instance check: " +
                                  server.errorString());
        return 1;
    }

    MainWindow w;
    w.show();

    QObject::connect(&a, &QApplication::aboutToQuit, [&w]() {
        if (w.torProcess())
            w.torProcess()->stopTor();
    });


    return a.exec();
}
