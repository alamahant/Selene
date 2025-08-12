#ifndef SIMPLEHTTPFILESERVER_H
#define SIMPLEHTTPFILESERVER_H

#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QDir>

/**
 * @brief Simple HTTP File Server using Qt only.
 *
 * Serves files from a directory and provides a basic HTML index at "/".
 */
class SimpleHttpFileServer : public QObject
{
    Q_OBJECT
public:
    explicit SimpleHttpFileServer(const QString& directory, int port, QObject* parent = nullptr);
    ~SimpleHttpFileServer();

    bool start();
    void stop();
    bool isRunning() const;
    QString getDirectory() const;
    int getPort() const;
    void restart();


signals:
    void started();
    void stopped();
    void error(const QString& message);

private slots:
    void onNewConnection();
    void onReadyRead();

private:
    void handleRequest(QTcpSocket* socket);
    void sendDirectoryListing(QTcpSocket* socket, const QString& dirPath);
    void sendFile(QTcpSocket* socket, const QString& fileName);
    void sendNotFound(QTcpSocket* socket);
    bool m_running = false;

    QString m_directory;
    int m_port;
    QTcpServer* m_server;
    QHash<QTcpSocket*, QByteArray> m_buffers;
};



#endif // SIMPLEHTTPFILESERVER_H
