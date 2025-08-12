#ifndef TORPROCESS_H
#define TORPROCESS_H
#include <QProcess>
#include <QString>
#include "logger.h"
#include<QMessageBox>
class TorProcess
{
public:
    TorProcess();
    static bool startTor(const QString& configPath);
    static void stopTor();
    static bool isTorRunning();
    static QString findTorPath();
    static void cleanup();
    static void killExistingTorProcesses();
    static void setTorProcess(QProcess *newTorProcess);

    static TorProcess* torInstance;  // For the single TorProcess instance

public slots:
    void newTorCircuit();

private:
    static QProcess* torProcess;

    static QStringList getPossibleTorPaths();
public:

};

#endif // TORPROCESS_H
