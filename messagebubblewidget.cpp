#include "messagebubblewidget.h"

#include <QDateTime>
#include <QFrame>
#include<QJsonDocument>
#include<QJsonObject>
#include<QJsonArray>


MessageBubbleWidget::MessageBubbleWidget(const ChatMessage& message, const QString& senderName, QWidget *parent)
    : QWidget(parent), m_message(message), m_senderName(senderName), m_isFromMe(message.isFromMe),
    m_contentLabel(nullptr),
    m_timeLabel(nullptr),
    senderLabel(nullptr)
{

    // Main horizontal layout for alignment
    QHBoxLayout* mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(10, 5, 10, 5);

    // Vertical layout for sender label and bubble
    QVBoxLayout* verticalLayout = new QVBoxLayout();
    verticalLayout->setSpacing(2);
    verticalLayout->setContentsMargins(0, 0, 0, 0);

    // Sender label (only for received messages, but you can show for all if you want)
    if (!m_isFromMe) {
        senderLabel = new QLabel(senderName, this);
        senderLabel->setStyleSheet("font-size: 11px; color: #888; font-weight: bold;");
        verticalLayout->addWidget(senderLabel, 0, Qt::AlignLeft);
    }else{
        senderLabel = nullptr;
    }
    // Optionally, show "Me" for your own messages:
    // else {
    //     QLabel* senderLabel = new QLabel(senderName, this);
    //     senderLabel->setStyleSheet("font-size: 11px; color: #888;");
    //     verticalLayout->addWidget(senderLabel, 0, Qt::AlignRight);
    // }

    // Create bubble container
    QFrame* bubbleContainer = new QFrame(this);
    bubbleContainer->setMinimumWidth(0); // was 100
    bubbleContainer->setMaximumWidth(500); // was 400
    bubbleContainer->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Minimum);
    bubbleContainer->setStyleSheet(
        m_isFromMe
            ? "QFrame { background-color: #007AFF; border-radius: 18px; }"
            : "QFrame { background-color: #28A745; border-radius: 18px; }"
        );

    // Create bubble layout
    QVBoxLayout* bubbleLayout = new QVBoxLayout(bubbleContainer);
    bubbleLayout->setContentsMargins(12, 8, 12, 8);
    bubbleLayout->setSpacing(2);

    // Message content label
    m_contentLabel = new QLabel(message.content, bubbleContainer);

    m_contentLabel->setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard);
    m_contentLabel->setMinimumSize(0, 0); // 50,20
    m_contentLabel->setMaximumWidth(450); //350
    m_contentLabel->setWordWrap(true);
    m_contentLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
    m_contentLabel->setStyleSheet("color: white; font-size: 14px; font-weight: normal;");
    bubbleLayout->addWidget(m_contentLabel);

    // Time label
    m_timeLabel = new QLabel(message.timestamp.toString("hh:mm:ss"), bubbleContainer);
    m_timeLabel->setStyleSheet("color: rgba(255, 255, 255, 180); font-size: 11px;");
    m_timeLabel->setAlignment(Qt::AlignRight);
    m_timeLabel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    bubbleLayout->addWidget(m_timeLabel);

    // Add bubble to vertical layout
    verticalLayout->addWidget(bubbleContainer, 0, m_isFromMe ? Qt::AlignRight : Qt::AlignLeft);

    // Align the whole block left or right
    if (m_isFromMe) {
        mainLayout->addStretch();
        mainLayout->addLayout(verticalLayout);
    } else {
        mainLayout->addLayout(verticalLayout);
        mainLayout->addStretch();
    }

    setLayout(mainLayout);
}

void MessageBubbleWidget::paintEvent(QPaintEvent *event)
{
    QWidget::paintEvent(event);
}

QLabel *MessageBubbleWidget::timeLabel() const
{
    return m_timeLabel;
}

void MessageBubbleWidget::setFontSize(int size)
{
    if (m_contentLabel) {
        m_contentLabel->setStyleSheet(
            QString("color: white; font-weight: normal; font-size: %1pt;").arg(size)
            );
    }
    if (m_timeLabel) {
        m_timeLabel->setStyleSheet(
            QString("color: rgba(255,255,255,180); font-size: %1pt;").arg(std::max(8, size - 4))
            );
    }
    if (senderLabel) {
        senderLabel->setStyleSheet(
            QString("color: #888; font-weight: bold; font-size: %1pt;").arg(std::max(8, size - 4))
            );
    }
}



