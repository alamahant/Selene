#include "maildialog.h"
#include "mailmanager.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QComboBox>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QTabWidget>
#include <QListWidget>
#include <QTextBrowser>
#include <QInputDialog>
#include <QMessageBox>
#include <QJsonObject>
#include"constants.h"
#include<QFileDialog>

MailDialog::MailDialog(MailManager* manager, QWidget* parent)
    : QDialog(parent), m_manager(manager)
{
    setWindowTitle("Mail");
    resize(720, 520);

    auto* layout = new QVBoxLayout(this);

    auto* top = new QHBoxLayout();
    m_accounts = new QComboBox(this);
    m_newAccount = new QPushButton("New", this);
    m_deleteAccount = new QPushButton("Delete", this);
    top->addWidget(m_accounts, 1);
    top->addWidget(m_newAccount);
    top->addWidget(m_deleteAccount);
    layout->addLayout(top);

    m_tabs = new QTabWidget(this);

    auto* compose = new QWidget(this);
    auto* cf = new QFormLayout(compose);
    m_to = new QLineEdit(compose);
    m_subject = new QLineEdit(compose);
    m_body = new QPlainTextEdit(compose);
    m_send = new QPushButton("Send", compose);

    auto* sendRow = new QHBoxLayout();
    m_attach = new QPushButton(tr("Attach…"), compose);
    connect(m_attach, &QPushButton::clicked, this, &MailDialog::onAttach);
    sendRow->addWidget(m_attach);
    sendRow->addStretch(1);
    sendRow->addWidget(m_send);

    cf->addRow("To:", m_to);
    cf->addRow("Subject:", m_subject);
    cf->addRow("Body:", m_body);
    cf->addRow(sendRow);

    m_tabs->addTab(compose, "Compose");

    auto* sent = new QWidget(this);
    auto* sl = new QHBoxLayout(sent);
    m_sentList = new QListWidget(sent);
    m_sentView = new QTextBrowser(sent);
    sl->addWidget(m_sentList, 1);
    sl->addWidget(m_sentView, 2);
    m_tabs->addTab(sent, "Sent");

    auto* failed = new QWidget(this);
    auto* fl = new QHBoxLayout(failed);
    m_failedList = new QListWidget(failed);
    m_failedView = new QTextBrowser(failed);
    m_retry = new QPushButton("Retry Selected", failed);
    auto* flv = new QVBoxLayout();
    flv->addWidget(m_failedView);
    flv->addWidget(m_retry);
    fl->addWidget(m_failedList, 1);
    fl->addLayout(flv, 2);
    m_tabs->addTab(failed, "Failed");


    auto* inbox = new QWidget(this);
    auto* il = new QHBoxLayout(inbox);
    m_inboxList = new QListWidget(inbox);
    m_inboxView = new QTextBrowser(inbox);

    auto* right = new QVBoxLayout();
    m_listenButton = new QPushButton("Start Listening", inbox);
    right->addWidget(m_inboxView, 1);
    right->addWidget(m_listenButton);

    il->addWidget(m_inboxList, 1);
    il->addLayout(right, 2);
    m_tabs->addTab(inbox, "Inbox");

    layout->addWidget(m_tabs, 1);

    connect(m_manager, &MailManager::accountsChanged, this, &MailDialog::refreshAccounts);
    connect(m_manager, &MailManager::messagesChanged, this, &MailDialog::refreshMessages);
    connect(m_newAccount, &QPushButton::clicked, this, &MailDialog::onNewAccount);
    connect(m_deleteAccount, &QPushButton::clicked, this, &MailDialog::onDeleteAccount);
    connect(m_send, &QPushButton::clicked, this, &MailDialog::onComposeSend);
    connect(m_retry, &QPushButton::clicked, this, &MailDialog::onRetryFailed);
    connect(m_sentList, &QListWidget::currentRowChanged, this, [this](int row) {
        if (row < 0) return;
        const auto msgs = m_manager->sentMessages();
        if (row < msgs.size()) showMessage(m_sentView, msgs[row]);
    });
    connect(m_failedList, &QListWidget::currentRowChanged, this, [this](int row) {
        if (row < 0) return;
        const auto msgs = m_manager->failedMessages();
        if (row < msgs.size()) showMessage(m_failedView, msgs[row]);
    });

    connect(m_manager, &MailManager::mailDelivered, this,
            [this](const QString& to, const QString&) {
                emit requestPlayMailSound();
                QMessageBox::information(this, "Sent", "Delivered to " + to);
            });
    connect(m_manager, &MailManager::error, this, [this](const QString& msg) {
        QMessageBox::warning(this, "Send failed", msg);
    });

    connect(m_inboxList, &QListWidget::currentRowChanged, this, [this](int row) {
        if (row < 0) return;
        const auto msgs = m_manager->inboxMessages();
        if (row < msgs.size()) showMessage(m_inboxView, msgs[row]);
    });
    connect(m_listenButton, &QPushButton::clicked, this, &MailDialog::onToggleListen);
    connect(m_manager, &MailManager::receiverRunningChanged,
            this, &MailDialog::onReceiverRunningChanged);
    connect(m_manager, &MailManager::receiverError,
            this, &MailDialog::onReceiverError);



    onReceiverRunningChanged(m_manager->isReceiverRunning());
    refreshAccounts();
    refreshMessages();

}

void MailDialog::refreshAccounts()
{
    m_accounts->clear();
    for (const auto& a : m_manager->accounts())
        m_accounts->addItem(a.address());
    const QString active = m_manager->activeAddress();
    if (!active.isEmpty()) m_accounts->setCurrentText(active);

    m_send->setEnabled(!m_manager->accounts().isEmpty());
    connect(m_accounts, &QComboBox::currentTextChanged, m_manager, &MailManager::setActiveAccount, Qt::UniqueConnection);
}

void MailDialog::refreshMessages()
{
    m_sentList->clear();
    for (const auto& m : m_manager->sentMessages())
        m_sentList->addItem(m["date"].toString() + "  " + m["to"].toString());

    m_failedList->clear();
    for (const auto& m : m_manager->failedMessages())
        m_failedList->addItem(m["date"].toString() + "  " + m["to"].toString());

    m_inboxList->clear();
    for (const auto& m : m_manager->inboxMessages())
        m_inboxList->addItem(m["date"].toString() + "  " + m["from"].toString());
}

void MailDialog::onNewAccount()
{
    bool ok = false;
    const QString name = QInputDialog::getText(this, "New Mail Address",
                                               "Username:", QLineEdit::Normal, {}, &ok);
    if (!ok || name.trimmed().isEmpty()) return;
    QString err;
    if (!m_manager->createAccount(name.trimmed(), &err))
        QMessageBox::warning(this, "Mail", err);
}

void MailDialog::onDeleteAccount()
{
    const QString addr = m_accounts->currentText();
    if (addr.isEmpty()) return;
    if (QMessageBox::question(this, "Delete",
            "Remove " + addr + "? The onion address will be lost.") != QMessageBox::Yes)
        return;
    QString err;
    if (!m_manager->removeAccount(addr, &err))
        QMessageBox::warning(this, "Mail", err);
}

void MailDialog::onComposeSend()
{
    QString err;
    m_manager->send(m_to->text().trimmed(), m_subject->text(), m_body->toPlainText(), &err);
    if (!err.isEmpty()) {
        QMessageBox::warning(this, "Send failed", err);
        return;
    }
    m_to->clear(); m_subject->clear(); m_body->clear();
}

void MailDialog::onRetryFailed()
{
    const int row = m_failedList->currentRow();
    if (row < 0) return;
    const auto msgs = m_manager->failedMessages();
    if (row >= msgs.size()) return;
    m_manager->retryFailed(msgs[row]["id"].toString());
}

void MailDialog::showMessage(QTextBrowser* view, const QJsonObject& m)
{
    view->setPlainText(QString("From: %1\nTo: %2\nDate: %3\nSubject: %4\n\n%5")
        .arg(m["from"].toString(), m["to"].toString(), m["date"].toString(),
             m["subject"].toString(), m["body"].toString()));
}


void MailDialog::onToggleListen()
{
    if (m_manager->isReceiverRunning())
        m_manager->stopReceiver();
    else
        m_manager->startReceiver(2525);
}

void MailDialog::onReceiverRunningChanged(bool running)
{
    m_listenButton->setText(running ? "Stop Listening" : "Start Listening");
}

void MailDialog::onReceiverError(const QString& message)
{
    QMessageBox::warning(this, "Receiver", message);
}


void MailDialog::onAttach()
{
    if (httpOnionAddress.isEmpty()) {
        QMessageBox::warning(this, tr("Attach"),
            tr("HTTP onion not ready yet."));
        return;
    }

    QStringList filePaths = QFileDialog::getOpenFileNames(
        this, tr("Select File(s) to Attach"), getFileSaveDirPath());
    if (filePaths.isEmpty()) return;

    const QString uuid = QUuid::createUuid().toString(QUuid::WithoutBraces);
    const QString destDir = getWWWDir() + QDir::separator() + uuid;
    QDir().mkpath(destDir);

    for (const QString& filePath : filePaths) {
        QFileInfo fi(filePath);
        const QString dest = destDir + QDir::separator() + fi.fileName();
        if (!QFile::copy(filePath, dest)) {
            QMessageBox::warning(this, tr("File Copy Error"),
                tr("Failed to copy %1").arg(filePath));
            QDir(destDir).removeRecursively();
            return;
        }
    }

    const QString url = httpOnionAddress + "/" + uuid + "/";
    m_body->appendPlainText("\nAttachments available at: " + url + "\n");
    emit requestHttpServerStart();

}
