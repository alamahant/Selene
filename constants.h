#ifndef CONSTANTS_H
#define CONSTANTS_H
#include<QStandardPaths>
#include<QDir>
#include<QString>
#include<QSettings>

extern const char* APP_VERSION;

extern  int DefaultChatServicePort;
extern  int DefaultHttpServicePort;
extern  int DefaultControlPort;
extern  int DefaultProxyPort;
extern const qreal DEFAULTFONTSIZE;
extern qreal FONTSIZE;
extern QString httpOnionAddress;
extern int connectionTimeout;
extern int circuitSettleTimeout;

QString getAppDataDir();
QString getWebsitesDir();

QString getChatHistoryFilePath();

QString getFileSaveDirPath();
QString getContactsDirPath();

QString getWWWDir();
QString getTorHttpHiddenDirPath();
QString getTorChatHiddenDirPath();
QString getCryptoDirPath();
QString getTorrcDirPath();
QString getDocumentsDirPath();
QString getDownloadsDirPath();

enum class SoundType {
    Connected,
    Disconnected,
    IncomingConnection,
    MessageReceived,
    MessageSent,
    UnknownContact
};

QString soundResourcePath(SoundType type);

#endif // CONSTANTS_H
