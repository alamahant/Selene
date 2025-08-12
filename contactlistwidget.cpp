#include "contactlistwidget.h"

#include <QScrollBar>

ContactListWidget::ContactListWidget(QWidget *parent)
    : QWidget(parent)


{
    setupUi();
}

void ContactListWidget::setupUi()
{
    mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(10);

    // Create a scroll area
    scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    // Create content widget for the scroll area
    contentWidget = new QWidget(scrollArea);
    QVBoxLayout* contentLayout = new QVBoxLayout(contentWidget);
    contentLayout->setContentsMargins(10, 10, 10, 10);
    contentLayout->setSpacing(10);
    contentLayout->addStretch();

    scrollArea->setWidget(contentWidget);
    mainLayout->addWidget(scrollArea);

    setLayout(mainLayout);

}

void ContactListWidget::editContactName(const Contact &contact)
{   QString friendlyName = contact.friendlyName;

    if (contactCards.contains(contact.onionAddress)) {
        contactCards[contact.onionAddress]->updateCardDisplay(friendlyName);
        // Update the card's display with new contact info
    }
}

void ContactListWidget::blockContact(const Contact &contact)
{ if (contactCards.contains(contact.onionAddress)) {
        // Update the card's visual state to show blocked status
        contactCards[contact.onionAddress]->setEnabled(false);
    }

}

void ContactListWidget::deleteContact(const Contact &contact)
{
    if (contactCards.contains(contact.onionAddress)) {
        ContactCardWidget* card = contactCards[contact.onionAddress];
        contactCards.remove(contact.onionAddress);
        delete card;
    }
}

void ContactListWidget::addContact(const Contact& contact)
{
    if (contactCards.contains(contact.onionAddress)) {
        return;
    }

    ContactCardWidget* card = new ContactCardWidget(contact, contentWidget);
    if (contact.friendlyName.contains("Me")) {
        QList<QPushButton*> buttons = card->findChildren<QPushButton*>();
        for(QPushButton* btn : buttons) {
            btn->setVisible(false);
        }

    }
    // Insert before the stretch
    QVBoxLayout* contentLayout = qobject_cast<QVBoxLayout*>(contentWidget->layout());

    contentLayout->insertWidget(contentLayout->count() - 1, card);


    // Connect signals
    connect(card, &ContactCardWidget::selected, this, &ContactListWidget::onContactSelected);
    connect(card, &ContactCardWidget::checkAvailabilityRequested, [this, onion = contact.onionAddress]() {
        emit connectRequested(onion);
    });

    connect(card, &ContactCardWidget::editRequested, this, &ContactListWidget::editContactRequested);
    connect(card, &ContactCardWidget::blockRequested, this, &ContactListWidget::blockContactRequested);
    connect(card, &ContactCardWidget::deleteRequested, this, &ContactListWidget::deleteContactRequested);



    contactCards[contact.onionAddress] = card;
    //card->updateStatusDisplay();

}


/*
void ContactListWidget::updateContactStatus(const QString& onionAddress, bool available)
{
    if (contactCards.contains(onionAddress)) {
                contactCards[onionAddress]->updateStatus(available);
    }
}
*/


void ContactListWidget::updateContactLastSeen(const QString& onionAddress, const QDateTime& lastSeen)
{
    if (contactCards.contains(onionAddress)) {
        contactCards[onionAddress]->updateLastSeen(lastSeen);
    }
}


void ContactListWidget::refreshContacts(const QMap<QString, Contact>& contacts)
{
    // Clear existing contacts
    for (auto card : contactCards) {
        delete card;
    }
    contactCards.clear();

    // Add all contacts
    for (auto it = contacts.begin(); it != contacts.end(); ++it) {
        addContact(it.value());

    }
}




void ContactListWidget::filterContacts(const QString& filter) {
    QVBoxLayout* contentLayout = qobject_cast<QVBoxLayout*>(contentWidget->layout());

    // Remove all cards from layout (but don't delete them)
    for (auto card : contactCards) {
        contentLayout->removeWidget(card);
    }

    if (filter.isEmpty()) {
        // Restore original layout
        for (auto card : contactCards) {
            contentLayout->insertWidget(contentLayout->count() - 1, card);
            card->show();
        }
    } else if (filter.startsWith("#b", Qt::CaseInsensitive)) {
        // Show only blocked contacts
        for (auto card : contactCards) {
            if (card->getIsBlocked()) {
                contentLayout->insertWidget(contentLayout->count() - 1, card);
                card->show();
            } else {
                card->hide();
            }
        }
    } else if (filter.startsWith("#c", Qt::CaseInsensitive)) {
        // Search in comments
        QString commentFilter = filter.mid(2).trimmed();
        for (auto card : contactCards) {
            if (card->getComments().contains(commentFilter, Qt::CaseInsensitive)) {
                contentLayout->insertWidget(contentLayout->count() - 1, card);
                card->show();
            } else {
                card->hide();
            }
        }
        //
    } else if (filter.startsWith("#o", Qt::CaseInsensitive)) {
        // filter by onion address
        QString onionFilter = filter.mid(2).trimmed();

        for (auto card : contactCards) {
            if (card->getOnionAddress().contains(onionFilter, Qt::CaseInsensitive)) {
                contentLayout->insertWidget(contentLayout->count() - 1, card);
                card->show();
            } else {
                card->hide();
            }
        }

    } else {
        // Default: filter by display name
        for (auto card : contactCards) {
            if (card->getDisplayName().contains(filter, Qt::CaseInsensitive)) {
                contentLayout->insertWidget(contentLayout->count() - 1, card);
                card->show();
            } else {
                card->hide();
            }
        }
    }
}




void ContactListWidget::onContactSelected()
{
    ContactCardWidget* card = qobject_cast<ContactCardWidget*>(sender());
    if (!card) return;

    // Deselect previous
    if (!selectedContact.isEmpty() && contactCards.contains(selectedContact)) {
        contactCards[selectedContact]->setSelected(false);
    }

    // Select new
    selectedContact = card->getOnionAddress();
    card->setSelected(true);

    emit contactSelected(selectedContact);
}

void ContactListWidget::setContactConnected(const QString& address) {
    if (contactCards.contains(address)) {
        contactCards[address]->setConnected(true);
    }
}

void ContactListWidget::setContactDisconnected(const QString& address) {
    if (contactCards.contains(address)) {
        contactCards[address]->setDisconnected();
    }
}


void ContactListWidget::setSelectedContact(const QString& onionAddress)
{
    // Deselect previous
    if (!selectedContact.isEmpty() && contactCards.contains(selectedContact)) {
        contactCards[selectedContact]->setSelected(false);
    }

    // Select new
    if (contactCards.contains(onionAddress)) {
        selectedContact = onionAddress;
        contactCards[onionAddress]->setSelected(true);
    }
}


