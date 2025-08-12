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

// Returns the application data directory
QString getAppDataDir();

// Returns the full path to the chat history file
QString getChatHistoryFilePath();

// Returns the directory where incoming files are saved
QString getFileSaveDirPath();
QString getContactsDirPath();

// Returns the directory for www files
QString getWWWDir();
//tor http direcory
QString getTorHttpHiddenDirPath();
QString getTorChatHiddenDirPath();
QString getCryptoDirPath();
QString getTorrcDirPath();
QString getConfigDirPath();
QString getDocumentsDirPath();

enum class SoundType {
    Connected,
    Disconnected,
    IncomingConnection,
    MessageReceived,
    MessageSent,
    UnknownContact
};

// Returns the resource path for a given SoundType
QString soundResourcePath(SoundType type);

#endif // CONSTANTS_H
