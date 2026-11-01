#pragma once

#include <QObject>
#include <QProcess>
#include <QString>
#include <QStringList>
#include"networkmanager.h"

class TorProcess : public QObject
{
    Q_OBJECT

public:
    explicit TorProcess(QObject* parent = nullptr);
    ~TorProcess() override;

    bool startTor(const QString& configPath);
    void stopTor();
    bool isTorRunning() const;
    void cleanup();
    void killExistingTorProcesses();
    void newTorCircuit();

    static QString findTorPath();
    static QStringList getPossibleTorPaths();

signals:
    void bootstrapProgress(int percent);
    void torReady();

private slots:
    void trackBootstrapProgress();

private:
    void killStoredTorPid();
    static int parseProgress(const QString& response);

    QProcess* m_torProcess = nullptr;

public:
    void setNetworkManager(NetworkManager* nm);
private:
    NetworkManager* m_networkManager = nullptr;
};
