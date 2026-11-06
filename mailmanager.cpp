#include "mailmanager.h"
#include "mailworker.h"
#include"constants.h"
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QProcess>
#include <QUuid>
#include <QDateTime>
#include <QDebug>
#include<QElapsedTimer>
#include<QThread>
#include<QFileInfo>
#include <QFileSystemWatcher>
MailManager::MailManager(TorConfig* torConfig, QObject* parent)
    : QObject(parent)
    , m_torConfig(torConfig)
{

    ensureFolders();
    load();
    auto* watcher = new QFileSystemWatcher(this);
    watcher->addPath(inboxDir());
    connect(watcher, &QFileSystemWatcher::directoryChanged, this, [this]() {
        const int n = QDir(inboxDir()).entryList({ "*.eml" }, QDir::Files).size();
        if (n > m_lastInboxCount) {
            emit messageReceived(n - m_lastInboxCount);
        }
        m_lastInboxCount = n;
        emit inboxChanged();
        emit messagesChanged();
    });
    m_lastInboxCount = QDir(inboxDir()).entryList({ "*.eml" }, QDir::Files).size();
}

MailManager::~MailManager()
{
}


QString MailManager::baseDir()
{
    return getDocumentsDirPath() + "/email";
}

QString MailManager::mailDir()  { return baseDir() + "/mail"; }
QString MailManager::emailDir() { return baseDir() + "/email"; }
QString MailManager::sentDir()  { return emailDir() + "/sent"; }
QString MailManager::failedDir(){ return emailDir() + "/failed"; }
QString MailManager::queueDir() { return emailDir() + "/queue"; }
QString MailManager::inboxDir() { return emailDir() + "/inbox"; }
QString MailManager::accountsPath(){ return mailDir() + "/accounts.json"; }


void MailManager::ensureFolders()
{
    QDir().mkpath(baseDir());
    QDir().mkpath(mailDir());
    QDir().mkpath(sentDir());
    QDir().mkpath(failedDir());
    QDir().mkpath(queueDir());
    QDir().mkpath(inboxDir());
}



QString MailManager::activeAddress() const { return m_active; }

void MailManager::setActiveAccount(const QString& address)
{
    if (address == m_active) return;
    m_active = address;
    save();
    emit accountsChanged();
}

bool MailManager::createAccount(const QString& username, QString* error)
{
    if (username.isEmpty()) { *error = "Username is empty."; return false; }
    static const QRegularExpression re("^[A-Za-z0-9._-]+$");
    if (!re.match(username).hasMatch()) {
        *error = "Username may only contain letters, digits, '.', '_', '-'.";
        return false;
    }
    if (!m_torConfig) { *error = "Tor config not available."; return false; }

    const QString onion = m_torConfig->getMailOnionAddress();
    if (onion.isEmpty()) { *error = "Mail hidden service not ready."; return false; }

    MailAccount acc;
    acc.username         = username;
    acc.onion            = onion;
    acc.hiddenServiceDir = m_torConfig->getMailHiddenServiceDir();

    m_accounts.append(acc);
    if (m_active.isEmpty()) m_active = acc.address();
    save();
    emit accountsChanged();
    return true;
}

bool MailManager::removeAccount(const QString& address, QString* error)
{
    int idx = -1;
    for (int i = 0; i < m_accounts.size(); ++i)
        if (m_accounts[i].address() == address) { idx = i; break; }
    if (idx < 0) { *error = "Account not found."; return false; }

    MailAccount acc = m_accounts.takeAt(idx);

    if (m_active == address)
        m_active = m_accounts.isEmpty() ? QString() : m_accounts.first().address();
    save();
    emit accountsChanged();
    return true;
}


static QList<QJsonObject> readJsonDir(const QString& dir)
{
    QList<QJsonObject> out;
    QDir d(dir);
    for (const QString& name : d.entryList({"*.json"}, QDir::Files)) {
        QFile f(d.filePath(name));
        if (!f.open(QIODevice::ReadOnly)) continue;
        const auto doc = QJsonDocument::fromJson(f.readAll());
        if (doc.isObject()) out.append(doc.object());
    }
    std::sort(out.begin(), out.end(), [](const QJsonObject& a, const QJsonObject& b){
        return a["date"].toString() > b["date"].toString();
    });
    return out;
}

static QJsonObject parseEml(const QString& path)
{
    QFile f(path);
    QJsonObject o;
    if (!f.open(QIODevice::ReadOnly)) return o;
    const QByteArray raw = f.readAll();
    f.close();

    int split = raw.indexOf("\r\n\r\n");
    if (split < 0) split = raw.indexOf("\n\n");
    QByteArray headers = split >= 0 ? raw.left(split) : raw;
    QByteArray body    = split >= 0 ? raw.mid(split + 4) : QByteArray();

    headers.replace("\r\n ", " ");
    headers.replace("\n ", " ");

    for (const QByteArray& line : headers.split('\n')) {
        const int colon = line.indexOf(':');
        if (colon < 0) continue;
        const QString key = QString::fromUtf8(line.left(colon)).trimmed().toLower();
        const QString val = QString::fromUtf8(line.mid(colon + 1)).trimmed();
        if (key == "from")            o["from"]    = val;
        else if (key == "to")         o["to"]      = val;
        else if (key == "subject")    o["subject"] = val;
        else if (key == "message-id") o["id"]      = val;
    }
    o["body"] = QString::fromUtf8(body).trimmed();
    o["date"] = QFileInfo(path).lastModified().toString(Qt::ISODate);
    if (!o.contains("id")) o["id"] = QFileInfo(path).baseName();
    return o;
}

QList<QJsonObject> MailManager::sentMessages() const   { return readJsonDir(sentDir()); }
QList<QJsonObject> MailManager::failedMessages() const { return readJsonDir(failedDir()); }
QList<QJsonObject> MailManager::queueMessages() const  { return readJsonDir(queueDir()); }

QList<QJsonObject> MailManager::inboxMessages() const
{
    QList<QJsonObject> out;
    QDir d(inboxDir());
    for (const QString& name : d.entryList({ "*.eml" }, QDir::Files))
        out.append(parseEml(d.filePath(name)));
    std::sort(out.begin(), out.end(), [](const QJsonObject& a, const QJsonObject& b) {
        return a["date"].toString() > b["date"].toString();
    });
    return out;
}


void MailManager::send(const QString& to, const QString& subject,
                       const QString& body, QString* error)
{
    if (!hasAccounts()) { *error = "No mail account."; return; }
    const QString at = "@";
    if (!to.contains(at)) { *error = "Invalid address: missing '@'."; return; }
    const QString server = to.section(at, 1);
    if (!server.endsWith(".onion")) { *error = "Only .onion recipients are supported."; return; }
    if (to.section(at, 0, 0).isEmpty()) { *error = "Invalid address: empty local part."; return; }
    if (!m_worker) { *error = "Worker not configured."; return; }

    MailItem item;
    item.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    item.from = m_active;
    item.to = to;
    item.subject = subject;
    item.body = body;
    item.date = QDateTime::currentDateTime().toString(Qt::ISODate);

    QMetaObject::invokeMethod(m_worker, "send", Qt::QueuedConnection, Q_ARG(MailItem, item));
}

void MailManager::retryFailed(const QString& id)
{
    QFile f(failedDir() + "/" + id + ".json");
    if (!f.open(QIODevice::ReadOnly)) return;
    const auto doc = QJsonDocument::fromJson(f.readAll());
    f.close();
    if (!doc.isObject()) return;
    const auto o = doc.object();

    MailItem item;
    item.id = id;
    item.from = o["from"].toString();
    item.to = o["to"].toString();
    item.subject = o["subject"].toString();
    item.body = o["body"].toString();
    item.date = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
    if (m_worker)
        QMetaObject::invokeMethod(m_worker, "send", Qt::QueuedConnection, Q_ARG(MailItem, item));
}


static void writeJson(const QString& dir, const QString& id, const QJsonObject& o)
{
    QDir().mkpath(dir);
    QFile f(dir + "/" + id + ".json");
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) return;
    f.write(QJsonDocument(o).toJson(QJsonDocument::Indented));
}

void MailManager::onMailSent(const MailItem& item)
{
    QJsonObject o;
    o["id"] = item.id; o["from"] = item.from; o["to"] = item.to;
    o["subject"] = item.subject; o["body"] = item.body;
    o["date"] = item.date; o["status"] = "sent";
    writeJson(sentDir(), item.id, o);
    QFile::remove(failedDir() + "/" + item.id + ".json");
    emit messagesChanged();
    emit mailDelivered(item.to, item.subject);
}

void MailManager::onMailFailed(const MailItem& item, const QString& error)
{
    QJsonObject o;
    o["id"] = item.id; o["from"] = item.from; o["to"] = item.to;
    o["subject"] = item.subject; o["body"] = item.body;
    o["date"] = item.date; o["status"] = "failed"; o["error"] = error;
    writeJson(failedDir(), item.id, o);
    emit messagesChanged();
    emit this->error(error);
}


void MailManager::load()
{
    m_accounts.clear();
    m_active.clear();
    QFile f(accountsPath());
    if (!f.open(QIODevice::ReadOnly)) return;
    const auto doc = QJsonDocument::fromJson(f.readAll());
    if (!doc.isObject()) return;
    const auto o = doc.object();
    m_active = o["active"].toString();
    for (const auto& v : o["accounts"].toArray()) {
        const auto a = v.toObject();
        MailAccount acc;
        acc.username = a["username"].toString();
        acc.onion = a["onion"].toString();
        acc.hiddenServiceDir = a["hiddenServiceDir"].toString();
        if (acc.isValid()) m_accounts.append(acc);
    }
}

void MailManager::save() const
{
    QJsonArray arr;
    for (const auto& acc : m_accounts) {
        QJsonObject a;
        a["username"] = acc.username;
        a["onion"] = acc.onion;
        a["hiddenServiceDir"] = acc.hiddenServiceDir;
        arr.append(a);
    }
    QJsonObject root;
    root["active"] = m_active;
    root["accounts"] = arr;

    QDir().mkpath(mailDir());
    QFile f(accountsPath());
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) return;
    f.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
}

void MailManager::startReceiver(int port)
{
    if (m_receiverRunning) return;
    if (!m_worker) { emit receiverError("Worker not configured."); return; }
    QMetaObject::invokeMethod(m_worker, "startReceiver", Qt::QueuedConnection,
                              Q_ARG(int, port),
                              Q_ARG(QString, inboxDir()));
}

void MailManager::stopReceiver()
{
    if (!m_worker) return;
    QMetaObject::invokeMethod(m_worker, "stopReceiver", Qt::QueuedConnection);
}

void MailManager::onReceiverStarted(bool ok, const QString& error)
{
    m_receiverRunning = ok;
    if (!ok) emit receiverError(error.isEmpty() ? "Receiver failed to start." : error);
    emit receiverRunningChanged(ok);
}

void MailManager::onReceiverStopped()
{
    m_receiverRunning = false;
    emit receiverRunningChanged(false);
}

