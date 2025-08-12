#include "logger.h"
#include "constants.h"

QFile Logger::logFile;
bool Logger::loggingEnabled = true;

bool Logger::initialized = false;


Logger::Logger() {}



void Logger::init() {
    if (initialized) {
        return;
    }

    QString appDirPath = getAppDataDir();


    QDir appDir(appDirPath);
    if (!appDir.exists())
        QDir().mkpath(appDirPath);


    QString logPath = appDir.filePath("chat.log");

    logFile.setFileName(logPath);

    if (!logFile.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        return;
    } else {
    }

    initialized = true;
}



void Logger::log(Level level, const QString& message)
{
    if (!loggingEnabled) return;

    if (!initialized) init();

    QString logEntry = QString("%1 [%2] %3\n")
                           .arg(getCurrentTimestamp())
                           .arg(levelToString(level))
                           .arg(message);

    QTextStream(&logFile) << logEntry;
    QTextStream(stdout) << logEntry;
    logFile.flush();
}

QString Logger::levelToString(Level level)
{
    switch (level) {
    case DEBUG: return "DEBUG";
    case INFO: return "INFO";
    case WARNING: return "WARNING";
    case ERROR: return "ERROR";
    default: return "UNKNOWN";
    }
}

QString Logger::getCurrentTimestamp()
{
    return QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz");
}

void Logger::setLoggingEnabled(bool enabled) {
    loggingEnabled = enabled;
}
