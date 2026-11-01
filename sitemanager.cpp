#include "sitemanager.h"

#include <QSettings>
#include <QDir>
#include <QFileInfo>
#include <QRegularExpression>
#include"logger.h"

static const char* kSettingsGroup = "sites";

SiteManager::SiteManager(QObject* parent)
    : QObject(parent)
{
    load();
}

void SiteManager::setFileShareRoot(const QString& root)
{
    QFileInfo fi(root);
    m_fileShareRoot = fi.canonicalFilePath();
    if (m_fileShareRoot.isEmpty())
        m_fileShareRoot = QDir(root).absolutePath();
}


QList<Site> SiteManager::sites() const
{
    QList<Site> out;
    out.reserve(m_sites.size());
    for (auto it = m_sites.constBegin(); it != m_sites.constEnd(); ++it)
        out.append(it.value());
    return out;
}


bool SiteManager::match(const QString& requestPath, Site& out) const
{
    QStringList prefixes = m_sites.keys();
    std::sort(prefixes.begin(), prefixes.end(),
              [](const QString& a, const QString& b){ return a.length() > b.length(); });

    for (const QString& p : prefixes) {
        const bool withSlash    = requestPath.startsWith(p);
        const bool withoutSlash = (requestPath + "/" == p);
        if (!withSlash && !withoutSlash)
            continue;

        out = m_sites[p];
        return true;
    }
    return false;
}





bool SiteManager::addSite(const QString& prefix, const QString& path, QString* error)
{
    auto fail = [&](const QString& msg) {
        if (error) *error = msg;
        return false;
    };

    const QString norm = normalizePrefix(prefix);
    if (!isValidPrefix(norm, error))
        return false;

    if (prefixExists(norm))
        return fail(QString("Prefix '%1' is already in use.").arg(norm));

    if (isNestedPrefix(norm))
        return fail(QString("Prefix '%1' nests inside an existing site prefix.").arg(norm));

    QFileInfo fi(path);
    if (!fi.exists() || !fi.isDir())
        return fail(QString("Path '%1' is not an existing directory.").arg(path));

    const QString canonical = fi.canonicalFilePath();
    if (canonical.isEmpty())
        return fail(QString("Cannot resolve canonical path for '%1'.").arg(path));

    if (pathAlreadyRegistered(canonical))
        return fail(QString("Folder '%1' is already registered as a site.").arg(canonical));

    if (overlapsFileShareRoot(canonical))
        return fail("Site folder overlaps the file-share root.");

    Site s;
    s.prefix  = norm;
    s.path    = canonical;
    s.enabled = true;
    insertAndPersist(s);
    Logger::log(Logger::INFO, QString("addSite: '%1' -> %2").arg(s.prefix, s.path));
    return true;
}

bool SiteManager::removeSite(const QString& prefix)
{
    const QString norm = normalizePrefix(prefix);
    if (!m_sites.contains(norm)){
        Logger::log(Logger::WARNING, QString("removeSite: '%1' not found").arg(norm));
        return false;
    }
    m_sites.remove(norm);
    Logger::log(Logger::INFO, QString("removeSite: '%1'").arg(norm));
    save();
    emit sitesChanged();
    return true;
}

bool SiteManager::setEnabled(const QString& prefix, bool enabled)
{
    const QString norm = normalizePrefix(prefix);
    auto it = m_sites.find(norm);
    if (it == m_sites.end())
        return false;
    if (it->enabled == enabled)
        return true;
    it->enabled = enabled;
    save();
    emit sitesChanged();
    return true;
}

bool SiteManager::renamePrefix(const QString& oldPrefix, const QString& newPrefix, QString* error)
{
    auto fail = [&](const QString& msg) {
        if (error) *error = msg;
        return false;
    };

    const QString oldNorm = normalizePrefix(oldPrefix);
    const QString newNorm = normalizePrefix(newPrefix);

    if (!m_sites.contains(oldNorm))
        return fail(QString("Prefix '%1' does not exist.").arg(oldNorm));
    if (oldNorm == newNorm)
        return true;
    if (!isValidPrefix(newNorm, error))
        return false;
    if (prefixExists(newNorm))
        return fail(QString("Prefix '%1' is already in use.").arg(newNorm));
    if (isNestedPrefix(newNorm))
        return fail(QString("Prefix '%1' nests inside an existing site prefix.").arg(newNorm));

    Site s = m_sites.take(oldNorm);
    s.prefix = newNorm;
    m_sites.insert(newNorm, s);
    save();
    emit sitesChanged();
    return true;
}


bool SiteManager::isValidPrefix(const QString& prefix, QString* error)
{
    auto fail = [&](const QString& msg) {
        if (error) *error = msg;
        return false;
    };

    if (prefix.isEmpty())
        return fail("Prefix is empty.");
    if (!prefix.startsWith('/') || !prefix.endsWith('/'))
        return fail("Prefix must start and end with '/'.");
    if (prefix.contains(".."))
        return fail("Prefix must not contain '..'.");
    if (prefix.contains("//"))
        return fail("Prefix must not contain '//'.");

    static const QRegularExpression re("^/[A-Za-z0-9._-]+/$");
    if (!re.match(prefix).hasMatch())
        return fail("Prefix may only contain letters, digits, '.', '_', '-'.");

    return true;
}

QString SiteManager::normalizePrefix(const QString& prefix)
{
    QString p = prefix.trimmed();
    if (p.isEmpty())
        return p;
    if (!p.startsWith('/'))
        p.prepend('/');
    if (!p.endsWith('/'))
        p.append('/');
    return p;
}

QString SiteManager::suggestPrefixFromPath(const QString& path)
{
    QString base = QFileInfo(path).fileName();
    if (base.isEmpty())
        base = "site";

    QString out;
    out.reserve(base.size());
    for (QChar c : base) {
        if (c.isLetterOrNumber() || c == '.' || c == '_' || c == '-')
            out.append(c.toLower());
        else
            out.append('-');
    }
    while (out.contains("--"))
        out.replace("--", "-");
    while (out.startsWith('-'))
        out.remove(0, 1);
    while (out.endsWith('-'))
        out.chop(1);
    if (out.isEmpty())
        out = "site";
    return "/" + out + "/";
}


bool SiteManager::prefixExists(const QString& prefix) const
{
    return m_sites.contains(prefix);
}

bool SiteManager::pathAlreadyRegistered(const QString& canonicalPath, const QString& exceptPrefix) const
{
    for (auto it = m_sites.constBegin(); it != m_sites.constEnd(); ++it) {
        if (it.key() == exceptPrefix)
            continue;
        if (it.value().path == canonicalPath)
            return true;
    }
    return false;
}

bool SiteManager::overlapsFileShareRoot(const QString& canonicalPath) const
{
    if (m_fileShareRoot.isEmpty() || canonicalPath.isEmpty())
        return false;
    const QString root = m_fileShareRoot.endsWith('/') ? m_fileShareRoot : m_fileShareRoot + '/';
    const QString site = canonicalPath.endsWith('/') ? canonicalPath : canonicalPath + '/';
    return site.startsWith(root) || root.startsWith(site);
}

bool SiteManager::isNestedPrefix(const QString& prefix) const
{
    for (const QString& existing : m_sites.keys()) {
        if (existing == prefix)
            continue;
        if (prefix.startsWith(existing) || existing.startsWith(prefix))
            return true;
    }
    return false;
}

void SiteManager::insertAndPersist(const Site& site)
{
    m_sites.insert(site.prefix, site);
    save();
    emit sitesChanged();
}


/*
void SiteManager::load()
{
    m_sites.clear();
    QSettings settings;
    settings.beginGroup(kSettingsGroup);

    const QStringList prefixes = settings.childGroups();
    for (const QString& prefix : prefixes) {
        settings.beginGroup(prefix);
        Site s;
        s.prefix  = prefix;
        s.path    = settings.value("path").toString();
        s.enabled = settings.value("enabled", true).toBool();
        settings.endGroup();

        if (!s.path.isEmpty())
            m_sites.insert(s.prefix, s);
    }

    settings.endGroup();
    emit sitesChanged();
}

void SiteManager::save() const
{
    QSettings settings;
    settings.remove(kSettingsGroup);   // clear stale entries
    settings.beginGroup(kSettingsGroup);
    for (auto it = m_sites.constBegin(); it != m_sites.constEnd(); ++it) {
        settings.beginGroup(it.key());
        settings.setValue("path", it.value().path);
        settings.setValue("enabled", it.value().enabled);
        settings.endGroup();
    }
    settings.endGroup();
}

*/

void SiteManager::load()
{
    m_sites.clear();
    QSettings settings;
    settings.beginGroup(kSettingsGroup);


    const QStringList keys = settings.childGroups();
    for (const QString& key : keys) {
        settings.beginGroup(key);
        Site s;
        s.prefix  = normalizePrefix(key);
        s.path    = settings.value("path").toString();
        s.enabled = settings.value("enabled", true).toBool();
        settings.endGroup();

        if (!s.path.isEmpty())
            m_sites.insert(s.prefix, s);
    }
    Logger::log(Logger::INFO, QString("SiteManager loaded %1 site(s)").arg(m_sites.size()));
    settings.endGroup();
    emit sitesChanged();
}


void SiteManager::save() const
{
    QSettings settings;
    settings.remove(kSettingsGroup);
    settings.sync();
    settings.beginGroup(kSettingsGroup);
    for (auto it = m_sites.constBegin(); it != m_sites.constEnd(); ++it) {
        QString key = it.key();
        if (key.startsWith('/')) key.remove(0, 1);
        if (key.endsWith('/'))   key.chop(1);

        settings.beginGroup(key);
        settings.setValue("path", it.value().path);
        settings.setValue("enabled", it.value().enabled);
        settings.endGroup();
    }
    settings.endGroup();
    settings.sync();
    if (settings.status() != QSettings::NoError) {
        Logger::log(Logger::ERROR, QString("save: QSettings error %1").arg(int(settings.status())));
    }
}

