#include "contactmanager.h"
#include<QMessageBox>
#include<QSystemTrayIcon>
#include<QApplication>
#include<QStyle>
#include"constants.h"
ContactManager::ContactManager(QObject *parent)
    : QObject{parent}
{
    contactsFilePath = getAppDataDir() + "/.selene/contacts.json";
    //contactsFilePath = "contacts.json";
    loadContacts();

}

void ContactManager::addContact(const Contact& newContact) {
    if (contacts.contains(newContact.onionAddress)) {
        QMessageBox::warning(nullptr, "Contact Exists",
                             "A contact with this onion address already exists.");
        return;
    }

    if (hasContactWithName(newContact.friendlyName)) {
        QMessageBox::warning(nullptr, "Name Exists",
                             "A contact with this friendly name already exists.");
        return;
    }

    contacts[newContact.onionAddress] = newContact;
    saveContacts();
    Logger::log(Logger::INFO, "Added contact: " + newContact.friendlyName +
                                  " (" + newContact.onionAddress + ")");

}

bool ContactManager::hasContactWithName(const QString& name) {
    for (const auto& contact : contacts) {
        if (contact.friendlyName == name) {
            return true;
        }
    }
    return false;
}



void ContactManager::removeContact(const QString& onion)
{
    if (contacts.contains(onion)) {
        contacts.remove(onion);
        saveContacts();
        Logger::log(Logger::INFO, "Removed contact: " + onion);
    }
}

QString ContactManager::getFriendlyName(const QString& onion) const
{
    if (contacts.contains(onion)) {
        return contacts[onion].friendlyName;
    }
    return onion; // Return onion address if no friendly name exists
}

bool ContactManager::isBlocked(const QString& onion) const
{
    return contacts.contains(onion) && contacts[onion].isBlocked;
}
/*
void ContactManager::setBlocked(const QString& onion, bool blocked)
{
    if (contacts.contains(onion)) {
        contacts[onion].isBlocked = blocked;
        saveContacts();
        Logger::log(Logger::INFO, QString("Contact %1 %2").arg(onion).arg(blocked ? "blocked" : "unblocked"));
    }
}
*/

void ContactManager::setBlocked(const QString& onion, bool blocked) {
    if (contacts.contains(onion)) {
        contacts[onion].isBlocked = blocked;
        if (blocked) {
            blockedContacts.insert(onion);
        } else {
            blockedContacts.remove(onion);
        }
        saveContacts();
        Logger::log(Logger::INFO, QString("Contact %1 %2").arg(onion).arg(blocked ? "blocked" : "unblocked"));
    }
}

QList<Contact> ContactManager::getBlockedContacts() const {
    QList<Contact> blocked;
    for (const auto& onion : blockedContacts) {
        blocked.append(contacts[onion]);
    }
    return blocked;
}

void ContactManager::saveContacts() const {
    QJsonObject root;
    for (auto it = contacts.constBegin(); it != contacts.constEnd(); ++it) {
        QJsonObject contactObj;
        contactObj["friendlyName"] = it.value().friendlyName;
        contactObj["publicKey"] = it.value().publicKey;
        contactObj["encryptionEnabled"] = it.value().encryptionEnabled;
        contactObj["isBlocked"] = it.value().isBlocked;
        contactObj["comments"] = it.value().comments;
        root[it.key()] = contactObj;
    }
    QJsonDocument doc(root);
    QFile file(contactsFilePath);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(doc.toJson());
        Logger::log(Logger::INFO, "Contacts saved to " + contactsFilePath);
    }
}

void ContactManager::loadContacts()
{
    QFile file(contactsFilePath);
    if (file.open(QIODevice::ReadOnly)) {
        QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
        QJsonObject root = doc.object();

        for (auto it = root.begin(); it != root.end(); ++it) {
            QString onion = it.key();
            QJsonObject contactObj = it.value().toObject();
            Contact contact(onion, contactObj["friendlyName"].toString());
            contact.isBlocked = contactObj["isBlocked"].toBool();
            contact.publicKey = contactObj["publicKey"].toString();
            contact.encryptionEnabled = contactObj["encryptionEnabled"].toBool();

            contact.comments = contactObj["comments"].toString();  // Added this line

            contacts[onion] = contact;
        }
        Logger::log(Logger::INFO, "Contacts loaded from " + contactsFilePath);
    }
}

void ContactManager::updateContact(const Contact& updatedContact) {
    contacts[updatedContact.onionAddress] = updatedContact;
    saveContacts();
   // emit contactsChanged(updatedContact);
}





void ContactManager::deleteContact(const QString& onion)
{
    contacts.remove(onion);
    saveContacts();
}

Contact ContactManager::getContact(const QString& onion) const {
    if (contacts.contains(onion)) {
        return contacts.value(onion);
    }
    // Return an empty contact if not found
    return Contact(onion);
}

