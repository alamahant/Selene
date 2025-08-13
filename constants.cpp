#include"constants.h"

/* hardcoded tor pports in case we need to revert to static ports
main.cpp:    //TorProcess::killExistingTorProcesses();
networkmanager.cpp:    //bool success = server->listen(QHostAddress::LocalHost, 8080);
mainwindow.cpp:    //httpServer = new SimpleHttpFileServer(docRootPath, 9090, this);
torprocess.cpp://socket.connectToHost("localhost", 9051);
networkmanager.cpp:    //proxy.setPort(9050);
*/

const char* APP_VERSION = "1.0.0";

int DefaultChatServicePort;
int DefaultHttpServicePort;
int DefaultControlPort;
int DefaultProxyPort;


QString getAppDataDir() {
    return QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
}

QString getChatHistoryFilePath() {
    QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    return dataDir + QDir::separator() + "chat_history.json";
}

QString getFileSaveDirPath() {
    QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    return dataDir + QDir::separator() + "files";
    //return getDocumentsDirPath() + QDir::separator() + "files";

}

QString getWWWDir() {
    QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    return dataDir + QDir::separator() + "www";
    //return getDocumentsDirPath() + QDir::separator() + "www";
}
QString getTorHttpHiddenDirPath(){
    return getAppDataDir() + QDir::separator() + ".selene" + QDir::separator() + "file_hidden_service";
}

QString getTorChatHiddenDirPath(){
    return getAppDataDir() + QDir::separator() + ".selene" + QDir::separator() + "hidden_service";
}

QString getCryptoDirPath(){
    return getAppDataDir() + QDir::separator() + ".keys";
}

QString getContactsDirPath(){
    return getAppDataDir() + QDir::separator() + "contacts";
    //return getDocumentsDirPath() + QDir::separator() + "contacts";
}

QString getTorrcDirPath(){
    return getAppDataDir() + QDir::separator() + ".selene" + QDir::separator() + "torrc";
}

QString getConfigDirPath(){
    return QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);;
}
QString getDocumentsDirPath(){
    return QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + QDir::separator() + "Selene";
}

QString soundResourcePath(SoundType type) {
    switch (type) {
    case SoundType::Connected:
        return QStringLiteral(":/sound/connected.wav");
    case SoundType::Disconnected:
        return QStringLiteral(":/sound/disconnected.wav");
    case SoundType::IncomingConnection:
        return QStringLiteral(":/sound/incomingConnection.wav");
    case SoundType::MessageReceived:
        return QStringLiteral(":/sound/messageReceived.wav");
    case SoundType::MessageSent:
        return QStringLiteral(":/sound/messageSent.wav");
    case SoundType::UnknownContact:
        return QStringLiteral(":/sound/unknownContact.wav");
    default:
        return QString();
    }
}

