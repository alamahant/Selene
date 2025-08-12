#include "contactcardwidget.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPainter>
#include <QMouseEvent>
#include <QIcon>
#include<QStyle>

ContactCardWidget::ContactCardWidget(const Contact& contact, QWidget *parent)
    : QWidget(parent),
    onionAddress(contact.onionAddress),
    displayName(contact.friendlyName),
    isSelected(false),
    isHovered(false),
    isBlocked(contact.isBlocked),// Initialize isBlocked
    comments(contact.comments)
{
    setupUi();
    updateBlockStatus(contact.isBlocked);

    updateStatusDisplay();

}

void ContactCardWidget::setupUi() {
    setMinimumHeight(70);
    setMaximumHeight(70);
    QHBoxLayout* mainLayout = new QHBoxLayout(this);

    // Status indicator (colored dot)
    statusDot = new QLabel(this);
    statusDot->setFixedSize(12, 12);
    statusDot->setProperty("status", "offline");
    statusDot->setStyleSheet("background-color: #ccc; border-radius: 6px;");
    mainLayout->addWidget(statusDot);

    // Contact info
    QVBoxLayout* infoLayout = new QVBoxLayout();
    nameLabel = new QLabel(displayName, this);
    QFont nameFont = nameLabel->font();
    nameFont.setBold(true);
    nameFont.setPointSize(nameFont.pointSize() + 1);
    nameLabel->setFont(nameFont);
    statusLabel = new QLabel(this);
    infoLayout->addWidget(nameLabel);
    infoLayout->addWidget(statusLabel);
    infoLayout->setSpacing(2);
    mainLayout->addLayout(infoLayout, 1);

    // Buttons layout
    QHBoxLayout* buttonsLayout = new QHBoxLayout();
    buttonsLayout->setSpacing(5);

    connectBtn = new QPushButton(this);
    connectBtn->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));
    connectBtn->setFixedSize(25, 25);
    connectBtn->setCursor(Qt::PointingHandCursor);
    connectBtn->setToolTip("Connect");
    buttonsLayout->addWidget(connectBtn);

    deleteBtn = new QPushButton(this);
    deleteBtn->setIcon(style()->standardIcon(QStyle::SP_TrashIcon));
    //deleteBtn->setIcon(style()->standardIcon(QStyle::SP_BrowserStop));

    deleteBtn->setFixedSize(25, 25);
    deleteBtn->setCursor(Qt::PointingHandCursor);
    deleteBtn->setToolTip("Delete Contact");
    buttonsLayout->addWidget(deleteBtn);

    editBtn = new QPushButton(this);
    editBtn->setIcon(style()->standardIcon(QStyle::SP_FileDialogDetailedView));
    editBtn->setFixedSize(25, 25);
    editBtn->setCursor(Qt::PointingHandCursor);
    editBtn->setToolTip("Edit Contact");
    buttonsLayout->addWidget(editBtn);

    blockBtn = new QPushButton(this);
    blockBtn->setCheckable(true);
    blockBtn->setToolTip("Block/Unblock Contact");
    connect(blockBtn, &QPushButton::clicked, [this]() {
        bool blocked = blockBtn->isChecked();
        updateBlockStatus(blocked);

        emit blockStatusChanged(onionAddress, blocked);
    });
    blockBtn->setIcon(style()->standardIcon(QStyle::SP_BrowserStop));
    //blockBtn->setIcon(style()->standardIcon(QStyle::SP_DialogCloseButton));

    blockBtn->setFixedSize(25, 25);
    blockBtn->setCursor(Qt::PointingHandCursor);
    buttonsLayout->addWidget(blockBtn);

    mainLayout->addLayout(buttonsLayout);
    mainLayout->setContentsMargins(10, 8, 10, 8);
    setLayout(mainLayout);

    setObjectName("contactCard");
    setStyleSheet("#contactCard { border-radius: 6px; }");

    connect(connectBtn, &QPushButton::clicked, this, &ContactCardWidget::checkAvailabilityRequested);
    connect(deleteBtn, &QPushButton::clicked, [this]() { emit deleteRequested(onionAddress); });
    connect(editBtn, &QPushButton::clicked, [this]() { emit editRequested(onionAddress); });
    connect(blockBtn, &QPushButton::clicked, [this]() { emit blockRequested(onionAddress); });
}

void ContactCardWidget::updateLastSeen(const QDateTime& lastSeen)
{
    lastSeenTime = lastSeen;
    updateStatusDisplay();
}

void ContactCardWidget::setConnected(bool connected) {
    if (connected) {
        connectBtn->setIcon(style()->standardIcon(QStyle::SP_MediaStop));
        connectBtn->setToolTip("Disconnect");
        statusDot->setStyleSheet("background-color: #4CAF50; border-radius: 6px;");
        statusLabel->setText("Connected");
    } else {
        //updateStatusDisplay();
    }
}


void ContactCardWidget::updateStatusDisplay()
{
    if (isBlocked) {
        statusDot->setStyleSheet("background-color: #d32f2f; border-radius: 6px;");
        statusLabel->setText("Blocked");
        return;
    } else {
        statusDot->setStyleSheet("background-color: #ccc; border-radius: 6px;");
        statusLabel->setText("Offline");
        }
}



void ContactCardWidget::setSelected(bool selected)
{
    isSelected = selected;
    update();
}

void ContactCardWidget::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // Draw background
    QColor bgColor;
    if (isSelected) {
        bgColor = QColor(230, 240, 255);
    } else if (isHovered) {
        bgColor = QColor(245, 245, 245);
    } else {
        bgColor = QColor(255, 255, 255);
    }

    painter.setPen(Qt::NoPen);
    painter.setBrush(bgColor);
    painter.drawRoundedRect(rect(), 6, 6);

    // Draw border if selected
    if (isSelected) {
        painter.setPen(QPen(QColor(100, 150, 255), 1));
        painter.setBrush(Qt::NoBrush);
        painter.drawRoundedRect(rect().adjusted(0, 0, -1, -1), 6, 6);
    }
}

void ContactCardWidget::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        emit selected();
    }
    QWidget::mousePressEvent(event);
}

void ContactCardWidget::enterEvent(QEnterEvent* event)
{
    isHovered = true;
    update();
    QWidget::enterEvent(event);
}

void ContactCardWidget::leaveEvent(QEvent* event)
{
    isHovered = false;
    update();
    QWidget::leaveEvent(event);
}

QLabel *ContactCardWidget::getStatusLabel() const
{
    return statusLabel;
}

QPushButton *ContactCardWidget::getBlockBtn() const
{
    return blockBtn;
}

QPushButton *ContactCardWidget::getEditBtn() const
{
    return editBtn;
}

QPushButton *ContactCardWidget::getDeleteBtn() const
{
    return deleteBtn;
}

QPushButton *ContactCardWidget::getConnectBtn() const
{
    return connectBtn;
}


void ContactCardWidget::setDisconnected() {
    connectBtn->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));
    connectBtn->setToolTip("Connect");
    statusDot->setStyleSheet("background-color: #FF5252; border-radius: 6px;");
    statusLabel->setText("Disconnected");
    // Start or restart the offline timer
    if (!offlineTimer) {
        offlineTimer = new QTimer(this);
        offlineTimer->setSingleShot(true);
        connect(offlineTimer, &QTimer::timeout, this, [this]() {
            statusDot->setStyleSheet("background-color: #ccc; border-radius: 6px;");
            statusLabel->setText("Offline");
        });
    }
    offlineTimer->start(30000); // 30 seconds
}

void ContactCardWidget::updateBlockStatus(bool blocked) {
    blockBtn->setChecked(blocked);
    if (blocked) {
        setStyleSheet("#contactCard { border-radius: 6px; background-color: #ffebee; }"); // Light red background
        nameLabel->setStyleSheet("color: #d32f2f;"); // Red text
        blockBtn->setIcon(style()->standardIcon(QStyle::SP_DialogApplyButton)); // Change icon to show unblock action
    } else {
        setStyleSheet("#contactCard { border-radius: 6px; }");
        nameLabel->setStyleSheet("");
        blockBtn->setIcon(style()->standardIcon(QStyle::SP_BrowserStop));
    }
}

void ContactCardWidget::updateCardDisplay(const QString &friendlyName)
{
    nameLabel->setText(friendlyName);
}

QString ContactCardWidget::getComments() const
{
    return comments;
}

bool ContactCardWidget::getIsBlocked() const
{
    return isBlocked;
}
/*
void ContactCardWidget::updateStatus(bool available)
{
    isAvailable = available;
    updateStatusDisplay();
}
*/
