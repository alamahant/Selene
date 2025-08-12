#ifndef CONTACTMANAGER_H
#define CONTACTMANAGER_H
#include "contact.h"
#include <QObject>
#include <QMap>
#include <QJsonDocument>
#include <QJsonObject>
#include <QFile>
#include "logger.h"
class ContactManager : public QObject
{
    Q_OBJECT
public:
    explicit ContactManager(QObject *parent = nullptr);
    void addContact(const Contact& updatedContact);
    void removeContact(const QString& onion);
    QString getFriendlyName(const QString& onion) const;
    bool isBlocked(const QString& onion) const;
    void setBlocked(const QString& onion, bool blocked);
    void saveContacts() const;
    void loadContacts();
    QMap<QString, Contact> getContacts() const { return contacts; }
    void updateContact(const Contact& updatedContact);

    void deleteContact(const QString& onion);
    Contact getContact(const QString& onion) const;
    bool isSelfContact(const QString& onion) {
        return getFriendlyName(onion).contains("Me");
    }
    QList<Contact> getBlockedContacts() const;


signals:
    void contactsChanged(const Contact& contact);

private:
    QMap<QString, Contact> contacts;  // onion -> Contact
    QString contactsFilePath;
    bool hasContactWithName(const QString& name);

    QSet<QString> blockedContacts;  // For quick lookup
};

#endif // CONTACTMANAGER_H
