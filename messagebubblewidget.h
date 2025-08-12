#ifndef MESSAGEBUBBLEWIDGET_H
#define MESSAGEBUBBLEWIDGET_H

#include <QWidget>
#include <QLabel>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPainter>
#include <QPainterPath>
#include<QDateTime>
#include<QTextBrowser>
#include"chatmessage.h"
#include"chatsession.h"



class MessageBubbleWidget : public QWidget
{
    Q_OBJECT

public:
    explicit MessageBubbleWidget(const ChatMessage& message, const QString& senderName, QWidget *parent = nullptr);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    ChatMessage m_message;
    QString m_senderName;
    QLabel* m_contentLabel;
    //QTextBrowser* m_contentLabel;

    QLabel* m_timeLabel;
    bool m_isFromMe;
    QLabel* senderLabel = nullptr;
public:
    void setFontSize(int size);


    QLabel *timeLabel() const;
};

#endif // MESSAGEBUBBLEWIDGET_H

