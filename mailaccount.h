#pragma once

#include <QString>
#include <QMetaType>

struct MailAccount {
    QString username;
    QString onion;
    QString hiddenServiceDir;

    QString address() const { return username + "@" + onion; }
    bool isValid() const { return !username.isEmpty() && !onion.isEmpty(); }
};

Q_DECLARE_METATYPE(MailAccount)