#include "simplehttpfileserver.h"

#include <QFile>
#include <QTextStream>
#include <QFileInfo>
#include <QDateTime>
#include <QDebug>
#include<QUrl>
#include"constants.h"
#include<QUuid>
#include"logger.h"

SimpleHttpFileServer::SimpleHttpFileServer(const QString& directory, int port, SiteManager *siteManager, QObject* parent)
    : QObject(parent), m_directory(directory), m_port(port), m_running(false), m_server(nullptr), m_siteManager(siteManager)
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

/*
void SimpleHttpFileServer::onReadyRead()
{
    QTcpSocket* socket = qobject_cast<QTcpSocket*>(sender());
    if (!socket)
        return;

    m_buffers[socket].append(socket->readAll());

    if (!m_buffers[socket].contains("\r\n\r\n"))
        return;

    handleRequest(socket);
    m_buffers.remove(socket);
}
*/

void SimpleHttpFileServer::onReadyRead()
{
    QTcpSocket* socket = qobject_cast<QTcpSocket*>(sender());
    if (!socket)
        return;

    m_buffers[socket].append(socket->readAll());

    const QByteArray& buf = m_buffers[socket];
    const int headerEnd = buf.indexOf("\r\n\r\n");
    if (headerEnd < 0)
        return;

    int contentLength = 0;
    const QByteArray headerBlock = buf.left(headerEnd);
    for (const QByteArray& line : headerBlock.split('\n')) {
        if (line.toLower().startsWith("content-length:"))
            contentLength = line.mid(15).trimmed().toInt();
    }

    if (buf.size() < headerEnd + 4 + contentLength)
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

    QString decodedPath = QUrl::fromPercentEncoding(path.toUtf8());

    if (decodedPath.contains("..")) {
        sendNotFound(socket);
        return;
    }

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
        sendFile(socket, requestedPath, false);
    } else {
        sendNotFound(socket);
    }
}
*/


void SimpleHttpFileServer::handleRequest(QTcpSocket* socket)
{
    QByteArray request = m_buffers.value(socket);
    QTextStream stream(request);
    QString line = stream.readLine();
    QString method, path;
    QTextStream lineStream(&line);
    lineStream >> method >> path;

    QString decodedPath = QUrl::fromPercentEncoding(path.toUtf8());

    if (method == "POST" && decodedPath.startsWith("/drop/")) {
        const int headerEnd = request.indexOf("\r\n\r\n");
        if (headerEnd < 0) { sendNotFound(socket); return; }
        const QByteArray headers = request.left(headerEnd);
        const QByteArray body = request.mid(headerEnd + 4);
        handleDropUpload(socket, headers, body, decodedPath);
        return;
    }

    if (method != "GET") {
        sendNotFound(socket);
        return;
    }




    if (decodedPath.contains("..")) {
        sendNotFound(socket);
        return;
    }

    Site site;
    bool hasSite = (m_siteManager != nullptr) && m_siteManager->match(decodedPath, site);



    if (hasSite && site.enabled && decodedPath + "/" == site.prefix) {
        QByteArray resp;
        resp += "HTTP/1.1 301 Moved Permanently\r\n";
        resp += "Location: " + site.prefix.toUtf8() + "\r\n";
        resp += "Content-Length: 0\r\n";
        resp += "Connection: close\r\n";
        resp += "\r\n";
        socket->write(resp);
        socket->disconnectFromHost();
        return;
    }


    QString baseDir;
    QString requestedPath;
    bool websiteMode = false;

    if (hasSite && site.enabled) {
        websiteMode = true;
        QString remainder;
        if (decodedPath + "/" == site.prefix)
            remainder = "/";
        else
            remainder = decodedPath.mid(site.prefix.length() - 1);
        baseDir = QDir(site.path).absolutePath();
        requestedPath = QDir::cleanPath(baseDir + "/" + remainder);
    } else {
        baseDir = QDir(m_directory).absolutePath();
        requestedPath = QDir::cleanPath(baseDir + "/" + decodedPath);
    }

    if (!requestedPath.startsWith(baseDir)) {
        sendNotFound(socket);
        return;
    }

    QFileInfo info(requestedPath);

    if (info.isDir()) {
        if (websiteMode) {
            QFileInfo index(QDir(requestedPath).filePath("index.html"));
            if (index.exists() && index.isFile())
                sendFile(socket, index.absoluteFilePath(), true);
            else
                sendNotFound(socket);
        } else {
            sendDirectoryListing(socket, requestedPath);
        }
    } else if (info.isFile()) {
        sendFile(socket, requestedPath, websiteMode);
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
        QString encodedFileUrl = QUrl::toPercentEncoding(fileUrl, "/");
        html += QString("<li><a href=\"%1\">%2</a></li>")
                   .arg(encodedFileUrl.toHtmlEscaped(), file.toHtmlEscaped());

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


void SimpleHttpFileServer::sendFile(QTcpSocket* socket, const QString& fullFilePath, bool websiteMode)
{
    QFile file(fullFilePath);
    if (!file.exists() || !file.open(QIODevice::ReadOnly)) {
        sendNotFound(socket);
        return;
    }

    QByteArray fileData = file.readAll();
    file.close();

    QString fileName = QFileInfo(file).fileName();

    QString contentType = "application/octet-stream";

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

    else if (fileName.endsWith(".exe") || fileName.endsWith(".msi"))
        contentType = "application/octet-stream"; // Forces download
    else if (fileName.endsWith(".deb"))
        contentType = "application/vnd.debian.binary-package";
    else if (fileName.endsWith(".rpm"))
        contentType = "application/x-rpm";
    else if (fileName.endsWith(".apk"))
        contentType = "application/vnd.android.package-archive";

    else if (fileName.endsWith(".ttf"))
        contentType = "font/ttf";
    else if (fileName.endsWith(".woff"))
        contentType = "font/woff";
    else if (fileName.endsWith(".woff2"))
        contentType = "font/woff2";


    bool renderable =
        contentType.startsWith("text/") ||
        contentType == "application/javascript" ||
        contentType == "application/json" ||
        contentType == "application/xml" ||
        contentType == "application/pdf" ||
        contentType.startsWith("image/") ||
        contentType.startsWith("audio/") ||
        contentType.startsWith("video/") ||
        contentType.startsWith("font/");

    bool forceDownload = !websiteMode || !renderable;
    QString disposition = forceDownload
        ? QString("attachment; filename=\"%1\"").arg(fileName)
        : QString("inline; filename=\"%1\"").arg(fileName);
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

void SimpleHttpFileServer::handleDropUpload(QTcpSocket* socket,
                                            const QByteArray& headers,
                                            const QByteArray& body,
                                            const QString& path)
{
    const QString uuid = path.section('/', 2, 2);
    if (uuid.isEmpty()) { sendNotFound(socket); return; }

    Site site;
    const bool hasSite = m_siteManager
        && m_siteManager->match("/" + uuid + "/", site)
        && site.enabled;
    if (!hasSite) { sendNotFound(socket); return; }

    QByteArray boundary;
    for (const QByteArray& line : headers.split('\n')) {
        if (line.toLower().contains("content-type:") && line.contains("boundary=")) {
            boundary = line.mid(line.indexOf("boundary=") + 9).trimmed();
            break;
        }
    }
    if (boundary.isEmpty()) { sendNotFound(socket); return; }

    const QByteArray delim = "--" + boundary;

    int partStart = body.indexOf(delim);
    if (partStart < 0) { sendNotFound(socket); return; }
    partStart += delim.size();
    if (body.mid(partStart, 2) == "\r\n")
        partStart += 2;

    int headerEnd = body.indexOf("\r\n\r\n", partStart);
    if (headerEnd < 0) { sendNotFound(socket); return; }
    const QByteArray partHeaders = body.mid(partStart, headerEnd - partStart);

    QString filename = "upload.bin";
    int fn = partHeaders.indexOf("filename=\"");
    if (fn >= 0) {
        fn += 10;
        int fnEnd = partHeaders.indexOf('"', fn);
        if (fnEnd > fn)
            filename = QString::fromUtf8(partHeaders.mid(fn, fnEnd - fn));
    }
    filename = QFileInfo(filename).fileName();
    if (filename.isEmpty())
        filename = "upload.bin";

    int dataStart = headerEnd + 4;
    int dataEnd = body.indexOf("\r\n" + delim, dataStart);
    if (dataEnd < 0)
        dataEnd = body.size();

    const QByteArray fileData = body.mid(dataStart, dataEnd - dataStart);

    QString outPath = getDownloadsDirPath() + "/" + filename;
    if (QFile::exists(outPath))
        outPath = getDownloadsDirPath() + "/" +
                  QUuid::createUuid().toString(QUuid::WithoutBraces) + "_" + filename;

    QFile out(outPath);
    if (!out.open(QIODevice::WriteOnly)) { sendNotFound(socket); return; }
    out.write(fileData);
    out.close();

    Logger::log(Logger::INFO,
        QString("Dropbox upload: '%1' (%2 bytes) -> %3")
            .arg(filename)
            .arg(fileData.size())
            .arg(outPath));

    QFile dropLog(getDownloadsDirPath() + "/dropbox.log");
    if (dropLog.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        QTextStream log(&dropLog);
        log << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss")
            << " | " << uuid
            << " | " << filename
            << " | " << fileData.size() << " bytes\n";
        dropLog.close();
    }

    const QByteArray resp =
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/html; charset=utf-8\r\n"
        "Connection: close\r\n"
        "\r\n"
        "<html><body><h1>Thanks</h1><p>File received: <code>"
        + filename.toHtmlEscaped().toUtf8() +
        "</code></p></body></html>";
    socket->write(resp);
    socket->disconnectFromHost();
}
