#pragma once

#include <QString>
#include <QMetaType>

struct MailItem {
    QString id;
    QString from;
    QString to;
    QString subject;
    QString body;
    QString date;
};

Q_DECLARE_METATYPE(MailItem)