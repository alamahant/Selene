#ifndef CONTACT_H
#define CONTACT_H
#include<QString>
#include<QDateTime>

class Contact {
public:
    QString onionAddress;
    QString friendlyName;
    bool isBlocked;
    QString comments;
    QDateTime lastSeen;
    QString publicKey;
    bool encryptionEnabled = false;
    QString group;

    Contact(const QString& onion = "", const QString& name = "") :
        onionAddress(onion),
        friendlyName(name),
        isBlocked(false) {}
};
#endif // CONTACT_H
