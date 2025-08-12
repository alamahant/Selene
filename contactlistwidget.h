#ifndef CONTACTLISTWIDGET_H
#define CONTACTLISTWIDGET_H

#include <QWidget>
#include <QVBoxLayout>
#include <QScrollArea>
#include <QMap>
#include "contactcardwidget.h"
#include "contact.h"

class ContactListWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ContactListWidget(QWidget *parent = nullptr);

    void addContact(const Contact& contact);

    //void updateContactStatus(const QString& onionAddress, bool available);
    void updateContactLastSeen(const QString& onionAddress, const QDateTime& lastSeen);
    void refreshContacts(const QMap<QString, Contact>& contacts);
    void filterContacts(const QString& filter);
    void setContactConnected(const QString& address);
    void setContactDisconnected(const QString& address);
signals:
    void contactSelected(const QString& onionAddress);
    void connectRequested(const QString& onionAddress);
    void editContactRequested(const QString& onionAddress);
    void blockContactRequested(const QString& onionAddress);
    void deleteContactRequested(const QString& onionAddress);


public slots:
    void onContactSelected();

private:
    QVBoxLayout* mainLayout;
    QWidget* contentWidget;
    QScrollArea* scrollArea;
    QMap<QString, ContactCardWidget*> contactCards;
    QString selectedContact;

    void setupUi();
public:
    void editContactName(const Contact& contact);
    void blockContact(const Contact& contact);
    void deleteContact(const Contact& contact);
    void setSelectedContact(const QString& onionAddress);

};

#endif // CONTACTLISTWIDGET_H

