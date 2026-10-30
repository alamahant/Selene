#pragma once

#include <QObject>
#include <QMap>
#include <QList>
#include <QString>

struct Site {
    QString prefix;      // normalized: "/blog/"
    QString path;        // absolute, canonical
    bool enabled = true;
};

class SiteManager : public QObject
{
    Q_OBJECT
public:
    explicit SiteManager(QObject* parent = nullptr);

    void setFileShareRoot(const QString& root);
    QString fileShareRoot() const { return m_fileShareRoot; }

    QList<Site> sites() const;

    bool addSite(const QString& prefix, const QString& path, QString* error = nullptr);
    bool removeSite(const QString& prefix);
    bool setEnabled(const QString& prefix, bool enabled);
    bool renamePrefix(const QString& oldPrefix, const QString& newPrefix, QString* error = nullptr);

    static bool isValidPrefix(const QString& prefix, QString* error = nullptr);
    static QString normalizePrefix(const QString& prefix);
    static QString suggestPrefixFromPath(const QString& path);

    void load();
    void save() const;

signals:
    void sitesChanged();

private:
    bool prefixExists(const QString& prefix) const;
    bool pathAlreadyRegistered(const QString& canonicalPath, const QString& exceptPrefix = QString()) const;
    bool overlapsFileShareRoot(const QString& canonicalPath) const;
    bool isNestedPrefix(const QString& prefix) const;
    void insertAndPersist(const Site& site);

    QMap<QString, Site> m_sites;
    QString m_fileShareRoot;

public:
    bool match(const QString& requestPath, Site& out) const;
};
