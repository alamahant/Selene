#ifndef LOGGER_H
#define LOGGER_H
#include <QString>
#include <QFile>
#include <QDateTime>
#include<QDir>
#include <QCoreApplication>
class Logger
{
public:     // Public types
    enum Level {
        DEBUG,
        INFO,
        WARNING,
        ERROR
    };

public:
    Logger();
    static void init();
    static void log(Level level, const QString& message);
    static void setLogFile(const QString& path);

private:    // Private data
    static QFile logFile;
    static bool initialized;

private:    // Private implementation
    static QString levelToString(Level level);
    static QString getCurrentTimestamp();
    static bool loggingEnabled;
public:
    static void setLoggingEnabled(bool enabled);

};

#endif // LOGGER_H
