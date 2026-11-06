#pragma once

#include <QDialog>

class MailManager;
class QComboBox;
class QLineEdit;
class QPlainTextEdit;
class QPushButton;
class QTabWidget;
class QListWidget;
class QTextBrowser;

class MailDialog : public QDialog
{
    Q_OBJECT
public:
    explicit MailDialog(MailManager* manager, QWidget* parent = nullptr);

private slots:
    void refreshAccounts();
    void refreshMessages();
    void onNewAccount();
    void onDeleteAccount();
    void onComposeSend();
    void onRetryFailed();
    void onToggleListen();
    void onReceiverRunningChanged(bool running);
    void onReceiverError(const QString& message);
    void onAttach();

signals:
    void requestHttpServerStart();
    void requestPlayMailSound();
private:
    void showMessage(QTextBrowser* view, const QJsonObject& msg);

    MailManager* m_manager;
    QComboBox* m_accounts = nullptr;
    QPushButton* m_newAccount = nullptr;
    QPushButton* m_deleteAccount = nullptr;

    QTabWidget* m_tabs = nullptr;

    QLineEdit* m_to = nullptr;
    QLineEdit* m_subject = nullptr;
    QPlainTextEdit* m_body = nullptr;
    QPushButton* m_send = nullptr;

    QListWidget* m_sentList = nullptr;
    QTextBrowser* m_sentView = nullptr;
    QListWidget* m_failedList = nullptr;
    QTextBrowser* m_failedView = nullptr;
    QPushButton* m_retry = nullptr;
    QListWidget* m_inboxList = nullptr;
    QTextBrowser* m_inboxView = nullptr;
    QPushButton* m_listenButton = nullptr;
    QPushButton* m_attach = nullptr;
};
