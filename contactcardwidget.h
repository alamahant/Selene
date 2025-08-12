#ifndef CONTACTCARDWIDGET_H
#define CONTACTCARDWIDGET_H

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QDateTime>
#include "contact.h"
#include<QTimer>

class ContactCardWidget : public QWidget
{
    Q_OBJECT
public:
    explicit ContactCardWidget(const Contact& contact, QWidget *parent = nullptr);

    QString getOnionAddress() const { return onionAddress; }
    QString getDisplayName() const { return displayName; }
    //void updateStatus(bool available);

    void updateLastSeen(const QDateTime& lastSeen);
    void setSelected(bool selected);
    void setConnected(bool connected);
    void setDisconnected();


    QPushButton *getConnectBtn() const;

    QPushButton *getDeleteBtn() const;

    QPushButton *getEditBtn() const;

    QPushButton *getBlockBtn() const;

    bool getIsBlocked() const;

    QString getComments() const;

    QLabel *getStatusLabel() const;
    void updateStatusDisplay();

signals:
    void selected();
    void checkAvailabilityRequested();
    void disconnectRequested();
    void editRequested(const QString& onionAddress);
    void blockRequested(const QString& onionAddress);
    void deleteRequested(const QString& onionAddress);
    void blockStatusChanged(const QString& onionAddress, bool isBlocked);
protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void enterEvent(QEnterEvent* event) override;
    void leaveEvent(QEvent* event) override;

private:
    QString onionAddress;
    QString displayName;
    bool isSelected;
    bool isHovered;
    //bool isAvailable;
    QDateTime lastSeenTime;

    QLabel* nameLabel;
    QLabel* statusLabel;
    QLabel* statusDot;
    QPushButton* connectBtn;
    QPushButton* deleteBtn;
    QPushButton* editBtn;
    QPushButton* blockBtn;

    void setupUi();
    void updateBlockStatus(bool blocked);
    bool isBlocked;
    QString comments; // Add this if not present
public:
    void updateCardDisplay(const QString &friendlyName);
private:
    QTimer* offlineTimer = nullptr;

};

#endif // CONTACTCARDWIDGET_H

