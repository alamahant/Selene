#include "dropboxdialog.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QDialogButtonBox>
#include <QMessageBox>
#include <QApplication>
#include <QClipboard>

DropboxDialog::DropboxDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Dropbox"));
    setMinimumWidth(460);
    setMinimumHeight(200);

    auto* layout = new QVBoxLayout(this);

    m_statusLabel = new QLabel(this);
    m_urlLabel = new QLabel(this);
    m_urlLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    m_urlLabel->setWordWrap(true);

    layout->addWidget(m_statusLabel);
    layout->addWidget(m_urlLabel);

    auto* buttons = new QHBoxLayout();
    m_newBtn = new QPushButton(tr("New UUID"), this);
    m_publishBtn = new QPushButton(tr("Publish"), this);
    m_unpublishBtn = new QPushButton(tr("Unpublish"), this);
    m_copyBtn = new QPushButton(tr("Copy URL"), this);
    buttons->addWidget(m_newBtn);
    buttons->addWidget(m_publishBtn);
    buttons->addWidget(m_unpublishBtn);
    buttons->addWidget(m_copyBtn);
    buttons->addStretch();
    layout->addLayout(buttons);

    auto* box = new QDialogButtonBox(QDialogButtonBox::Close, this);
    layout->addWidget(box);
    connect(box, &QDialogButtonBox::rejected, this, &QDialog::reject);

    connect(m_newBtn, &QPushButton::clicked, this, [this]() {
        if (m_state == NoPage) {
            emit newUuidRequested(false);
            return;
        }
        const auto r = QMessageBox::question(this, tr("New Dropbox"),
            tr("Remove the old drop page?\n\nThe old URL will stop working. "
               "Files already uploaded are not affected."),
            QMessageBox::Yes | QMessageBox::No);
        if (r == QMessageBox::Yes)
            emit newUuidRequested(true);
    });
    connect(m_publishBtn, &QPushButton::clicked, this, &DropboxDialog::publishRequested);
    connect(m_unpublishBtn, &QPushButton::clicked, this, &DropboxDialog::unpublishRequested);
    connect(m_copyBtn, &QPushButton::clicked, this, [this]() {
        if (!m_url.isEmpty())
            QApplication::clipboard()->setText(m_url);
    });

    setState(NoPage, {}, {});
}

void DropboxDialog::setState(State state, const QString& uuid, const QString& url)
{
    m_state = state;
    m_uuid = uuid;
    m_url = url;

    switch (state) {
    case NoPage:
        m_statusLabel->setText(tr("Status: No page"));
        m_urlLabel->setText(tr("URL: —"));
        break;
    case Unpublished:
        m_statusLabel->setText(tr("Status: Unpublished (%1…)").arg(uuid.left(8)));
        m_urlLabel->setText(tr("URL: —"));
        break;
    case Published:
        m_statusLabel->setText(tr("Status: Published (%1…)").arg(uuid.left(8)));
        m_urlLabel->setText(tr("URL: %1").arg(url));
        break;
    }

    m_newBtn->setEnabled(true);
    m_publishBtn->setEnabled(state == Unpublished);
    m_unpublishBtn->setEnabled(state == Published);
    m_copyBtn->setEnabled(state == Published);
}
