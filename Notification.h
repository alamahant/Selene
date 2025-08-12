#ifndef NOTIFICATION_H
#define NOTIFICATION_H

#include <QSystemTrayIcon>
#include <QApplication>
#include <QStyle>
#include <QString>

class Notification
{
public:

    Notification(const QString& title, const QString& text, int duration = 3000,
                 QSystemTrayIcon::MessageIcon icon = QSystemTrayIcon::Information)
    {
        // Use a static tray icon so it persists for the lifetime of the app
        static QSystemTrayIcon* tray = nullptr;
        if (!tray) {
            tray = new QSystemTrayIcon(QApplication::style()->standardIcon(QStyle::SP_MessageBoxInformation));
            tray->show();
        }
        tray->showMessage(title, text, icon, duration);
    }
};

#endif // NOTIFICATION_H
