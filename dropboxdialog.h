#pragma once

#include <QDialog>
#include <QString>

class QLabel;
class QPushButton;

class DropboxDialog : public QDialog
{
    Q_OBJECT
public:
    enum State { NoPage, Unpublished, Published };

    explicit DropboxDialog(QWidget* parent = nullptr);

    void setState(State state, const QString& uuid, const QString& url);

signals:
    void newUuidRequested(bool removeOld);
    void publishRequested();
    void unpublishRequested();

private:
    QLabel* m_statusLabel = nullptr;
    QLabel* m_urlLabel = nullptr;
    QPushButton* m_newBtn = nullptr;
    QPushButton* m_publishBtn = nullptr;
    QPushButton* m_unpublishBtn = nullptr;
    QPushButton* m_copyBtn = nullptr;

    QString m_uuid;
    QString m_url;
    State m_state = NoPage;
};