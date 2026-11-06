#include"constants.h"



const char* APP_VERSION = "1.0.2";

int DefaultChatServicePort;
int DefaultHttpServicePort;
int DefaultControlPort;
int DefaultProxyPort;
int connectionTimeout = 30;
int circuitSettleTimeout = 15;

#ifdef Q_OS_WIN
    const qreal DEFAULTFONTSIZE = 11;
#else
    const qreal DEFAULTFONTSIZE = 12;
#endif
qreal FONTSIZE = DEFAULTFONTSIZE;

QString httpOnionAddress = QString();

QString getAppDataDir() {
    return QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
}




QString getChatHistoryFilePath() {
    QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    return dataDir + QDir::separator() + "chat_history.json";
}

QString getFileSaveDirPath() {
    return getDocumentsDirPath() + QDir::separator() + "files";

}

QString getWWWDir() {
    return getDocumentsDirPath() + QDir::separator() + "www";
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
    return getDocumentsDirPath() + QDir::separator() + "contacts";
}

QString getTorrcDirPath(){
    return getAppDataDir() + QDir::separator() + ".selene" + QDir::separator() + "torrc";
}



QString getDocumentsDirPath(){
    return QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + QDir::separator() + "Selene";
}

QString getWebsitesDir() {
    return getDocumentsDirPath() + "/websites";
}


QString getDownloadsDirPath(){
    return getDocumentsDirPath() + QDir::separator() + "Downloads";
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
    case SoundType::EmailReceived:
        return QStringLiteral(":/sound/mail.wav");
    default:
        return QString();
    }
}

