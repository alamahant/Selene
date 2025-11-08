#include "simplehttpfileserver.h"

#include <QFile>
#include <QTextStream>
#include <QFileInfo>
#include <QDateTime>
#include <QDebug>
#include<QUrl>

SimpleHttpFileServer::SimpleHttpFileServer(const QString& directory, int port, QObject* parent)
    : QObject(parent), m_directory(directory), m_port(port), m_running(false), m_server(nullptr)
{
}

SimpleHttpFileServer::~SimpleHttpFileServer()
{
    stop();
}

bool SimpleHttpFileServer::start()
{
    if (m_running)
        return true;

    QDir dir(m_directory);
    if (!dir.exists()) {
        if (!dir.mkpath(".")) {
            emit error("Failed to create directory: " + m_directory);
            return false;
        }
    }

    m_server = new QTcpServer(this);
    connect(m_server, &QTcpServer::newConnection, this, &SimpleHttpFileServer::onNewConnection);

    if (!m_server->listen(QHostAddress::LocalHost, m_port)) {
        emit error("Failed to start HTTP server on port " + QString::number(m_port));
        m_server->deleteLater();
        m_server = nullptr;
        return false;
    }

    m_running = true;
    emit started();
    return true;
}

void SimpleHttpFileServer::stop()
{
    if (m_running && m_server) {
        m_server->close();
        m_server->deleteLater();
        m_server = nullptr;
        m_running = false;
        emit stopped();
    }
}

bool SimpleHttpFileServer::isRunning() const
{
    return m_running;
}

QString SimpleHttpFileServer::getDirectory() const
{
    return m_directory;
}

int SimpleHttpFileServer::getPort() const
{
    return m_port;
}

void SimpleHttpFileServer::onNewConnection()
{
    while (m_server->hasPendingConnections()) {
        QTcpSocket* socket = m_server->nextPendingConnection();
        connect(socket, &QTcpSocket::readyRead, this, &SimpleHttpFileServer::onReadyRead);
        connect(socket, &QTcpSocket::disconnected, socket, &QTcpSocket::deleteLater);
    }
}

void SimpleHttpFileServer::onReadyRead()
{
    QTcpSocket* socket = qobject_cast<QTcpSocket*>(sender());
    if (!socket)
        return;

    m_buffers[socket].append(socket->readAll());

    // Check if we have a full HTTP request (look for double CRLF)
    if (!m_buffers[socket].contains("\r\n\r\n"))
        return;

    handleRequest(socket);
    m_buffers.remove(socket);
}

/*
void SimpleHttpFileServer::handleRequest(QTcpSocket* socket)
{
    QByteArray request = m_buffers.value(socket);
    QTextStream stream(request);
    QString line = stream.readLine();
    QString method, path;
    QTextStream lineStream(&line);
    lineStream >> method >> path;

    if (method != "GET") {
        sendNotFound(socket);
        return;
    }

    if (path.contains("..")) {
        sendNotFound(socket);
        return;
    }

    QString requestedPath = QDir::cleanPath(m_directory + "/" + path);
    QString baseDir = QDir(m_directory).absolutePath();

    if (!requestedPath.startsWith(baseDir)) {
        sendNotFound(socket);
        return;
    }

    QFileInfo info(requestedPath);

    if (info.isDir()) {
        sendDirectoryListing(socket, requestedPath);
    } else if (info.isFile()) {
        sendFile(socket, requestedPath);
    } else {
        sendNotFound(socket);
    }
}
*/
//handlerequest version that protects against spaces in filenames
void SimpleHttpFileServer::handleRequest(QTcpSocket* socket)
{
    QByteArray request = m_buffers.value(socket);
    QTextStream stream(request);
    QString line = stream.readLine();
    QString method, path;
    QTextStream lineStream(&line);
    lineStream >> method >> path;

    if (method != "GET") {
        sendNotFound(socket);
        return;
    }

    // FIX: URL decode the path before using it!
    QString decodedPath = QUrl::fromPercentEncoding(path.toUtf8());

    if (decodedPath.contains("..")) {
        sendNotFound(socket);
        return;
    }

    // Use the DECODED path to find the file
    QString requestedPath = QDir::cleanPath(m_directory + "/" + decodedPath);
    QString baseDir = QDir(m_directory).absolutePath();

    if (!requestedPath.startsWith(baseDir)) {
        sendNotFound(socket);
        return;
    }

    QFileInfo info(requestedPath);

    if (info.isDir()) {
        sendDirectoryListing(socket, requestedPath);
    } else if (info.isFile()) {
        sendFile(socket, requestedPath);
    } else {
        sendNotFound(socket);
    }
}


void SimpleHttpFileServer::sendDirectoryListing(QTcpSocket* socket, const QString& dirPath)
{
    QDir dir(dirPath);
    if (!dir.exists()) {
        sendNotFound(socket);
        return;
    }

    QString relativePath = QDir(m_directory).relativeFilePath(dirPath);
    if (!relativePath.isEmpty() && !relativePath.endsWith('/')) {
        relativePath += '/';
    }

    QStringList files = dir.entryList(QDir::Files | QDir::NoDotAndDotDot, QDir::Name);

    QString html = "<html><head><title>Shared Files</title></head><body>";
    html += "<h1>Available Files</h1><ul>";

    for (const QString& file : files) {
        QString fileUrl = "/" + relativePath + file;  // Correct URL for subdir files
        // protect against spaces in urls
        QString encodedFileUrl = QUrl::toPercentEncoding(fileUrl, "/");
        html += QString("<li><a href=\"%1\">%2</a></li>")
                   .arg(encodedFileUrl.toHtmlEscaped(), file.toHtmlEscaped());

                   // .arg(fileUrl.toHtmlEscaped(), file.toHtmlEscaped());
    }

    html += "</ul></body></html>";

    QByteArray response;
    response += "HTTP/1.1 200 OK\r\n";
    response += "Content-Type: text/html; charset=utf-8\r\n";
    response += "Content-Length: " + QByteArray::number(html.toUtf8().size()) + "\r\n";
    response += "Connection: close\r\n";
    response += "\r\n";
    response += html.toUtf8();

    socket->write(response);
    socket->disconnectFromHost();
}


void SimpleHttpFileServer::sendFile(QTcpSocket* socket, const QString& fullFilePath)
{
    QFile file(fullFilePath);
    if (!file.exists() || !file.open(QIODevice::ReadOnly)) {
        sendNotFound(socket);
        return;
    }

    QByteArray fileData = file.readAll();
    file.close();

    QString fileName = QFileInfo(file).fileName();

    // Basic content type detection
    QString contentType = "application/octet-stream";

    // ===== Supported File Extensions =====
    // Text & Documents
    if (fileName.endsWith(".txt"))
        contentType = "text/plain";
    else if (fileName.endsWith(".html") || fileName.endsWith(".htm"))
        contentType = "text/html";
    else if (fileName.endsWith(".css"))
        contentType = "text/css";
    else if (fileName.endsWith(".js"))
        contentType = "application/javascript";
    else if (fileName.endsWith(".json"))
        contentType = "application/json";
    else if (fileName.endsWith(".xml"))
        contentType = "application/xml";
    else if (fileName.endsWith(".csv"))
        contentType = "text/csv";
    else if (fileName.endsWith(".pdf"))
        contentType = "application/pdf";

    // Images
    else if (fileName.endsWith(".jpg") || fileName.endsWith(".jpeg"))
        contentType = "image/jpeg";
    else if (fileName.endsWith(".png"))
        contentType = "image/png";
    else if (fileName.endsWith(".gif"))
        contentType = "image/gif";
    else if (fileName.endsWith(".svg"))
        contentType = "image/svg+xml";
    else if (fileName.endsWith(".webp"))
        contentType = "image/webp";
    else if (fileName.endsWith(".bmp"))
        contentType = "image/bmp";
    else if (fileName.endsWith(".ico"))
        contentType = "image/x-icon";

    // Audio
    else if (fileName.endsWith(".mp3"))
        contentType = "audio/mpeg";
    else if (fileName.endsWith(".wav"))
        contentType = "audio/wav";
    else if (fileName.endsWith(".ogg"))
        contentType = "audio/ogg";
    else if (fileName.endsWith(".flac"))
        contentType = "audio/flac";
    else if (fileName.endsWith(".m4a"))
        contentType = "audio/mp4";

    // Video
    else if (fileName.endsWith(".mp4"))
        contentType = "video/mp4";
    else if (fileName.endsWith(".webm"))
        contentType = "video/webm";
    else if (fileName.endsWith(".mkv"))
        contentType = "video/x-matroska";
    else if (fileName.endsWith(".avi"))
        contentType = "video/x-msvideo";
    else if (fileName.endsWith(".mov"))
        contentType = "video/quicktime";

    // Archives
    else if (fileName.endsWith(".zip"))
        contentType = "application/zip";
    else if (fileName.endsWith(".tar"))
        contentType = "application/x-tar";
    else if (fileName.endsWith(".gz") || fileName.endsWith(".tgz"))
        contentType = "application/gzip";
    else if (fileName.endsWith(".7z"))
        contentType = "application/x-7z-compressed";
    else if (fileName.endsWith(".rar"))
        contentType = "application/x-rar-compressed";

    // Office & Documents
    else if (fileName.endsWith(".doc") || fileName.endsWith(".docx"))
        contentType = "application/msword";
    else if (fileName.endsWith(".xls") || fileName.endsWith(".xlsx"))
        contentType = "application/vnd.ms-excel";
    else if (fileName.endsWith(".ppt") || fileName.endsWith(".pptx"))
        contentType = "application/vnd.ms-powerpoint";
    else if (fileName.endsWith(".odt"))
        contentType = "application/vnd.oasis.opendocument.text";
    else if (fileName.endsWith(".ods"))
        contentType = "application/vnd.oasis.opendocument.spreadsheet";

    // Executables & Binaries
    else if (fileName.endsWith(".exe") || fileName.endsWith(".msi"))
        contentType = "application/octet-stream"; // Forces download
    else if (fileName.endsWith(".deb"))
        contentType = "application/vnd.debian.binary-package";
    else if (fileName.endsWith(".rpm"))
        contentType = "application/x-rpm";
    else if (fileName.endsWith(".apk"))
        contentType = "application/vnd.android.package-archive";

    // Fonts
    else if (fileName.endsWith(".ttf"))
        contentType = "font/ttf";
    else if (fileName.endsWith(".woff"))
        contentType = "font/woff";
    else if (fileName.endsWith(".woff2"))
        contentType = "font/woff2";

    QString disposition = QString("attachment; filename=\"%1\"").arg(fileName);

    QByteArray response;
    response += "HTTP/1.1 200 OK\r\n";
    response += "Content-Type: " + contentType.toUtf8() + "\r\n";
    response += "Content-Disposition: " + disposition.toUtf8() + "\r\n";
    response += "Content-Length: " + QByteArray::number(fileData.size()) + "\r\n";
    response += "Connection: close\r\n";
    response += "\r\n";
    response += fileData;

    socket->write(response);
    socket->disconnectFromHost();
}



void SimpleHttpFileServer::sendNotFound(QTcpSocket* socket)
{
    QByteArray response;
    response += "HTTP/1.1 404 Not Found\r\n";
    response += "Content-Type: text/plain\r\n";
    response += "Content-Length: 13\r\n";
    response += "Connection: close\r\n";
    response += "\r\n";
    response += "404 Not Found";

    socket->write(response);
    socket->disconnectFromHost();
}


void SimpleHttpFileServer::restart()
{
    stop();
    start();
}
