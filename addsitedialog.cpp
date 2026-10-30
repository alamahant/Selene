#include "addsitedialog.h"
#include "constants.h"
#include "sitemanager.h"

#include <QFormLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QLabel>
#include <QPushButton>
#include <QFileDialog>
#include <QDialogButtonBox>
#include <QStandardPaths>
#include <QDir>

AddSiteDialog::AddSiteDialog(SiteManager* manager, QWidget* parent)
    : QDialog(parent), m_manager(manager)
{
    setWindowTitle(tr("Add Site"));
    setMinimumWidth(460);

    auto* layout = new QVBoxLayout(this);

    auto* form = new QFormLayout();

    auto* pathRow = new QHBoxLayout();
    m_pathEdit = new QLineEdit(this);
    m_pathEdit->setReadOnly(true);
    m_pathEdit->setPlaceholderText(tr("Choose a folder inside sites/"));
    auto* browseBtn = new QPushButton(tr("Browse…"), this);
    pathRow->addWidget(m_pathEdit);
    pathRow->addWidget(browseBtn);
    form->addRow(tr("Folder:"), pathRow);

    m_prefixEdit = new QLineEdit(this);
    m_prefixEdit->setPlaceholderText(tr("/blog/"));
    form->addRow(tr("Prefix:"), m_prefixEdit);

    m_previewLabel = new QLabel(this);
    m_previewLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    form->addRow(tr("Available at:"), m_previewLabel);

    layout->addLayout(form);

    m_errorLabel = new QLabel(this);
    m_errorLabel->setStyleSheet("color: #c0392b;");
    m_errorLabel->setWordWrap(true);
    m_errorLabel->hide();
    layout->addWidget(m_errorLabel);

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Cancel, this);
    m_addButton = buttons->addButton(tr("Add"), QDialogButtonBox::AcceptRole);
    m_addButton->setEnabled(false);
    layout->addWidget(buttons);

    connect(browseBtn, &QPushButton::clicked, this, &AddSiteDialog::onBrowse);
    connect(m_prefixEdit, &QLineEdit::textChanged, this, &AddSiteDialog::onPrefixChanged);
    connect(m_addButton, &QPushButton::clicked, this, &AddSiteDialog::onAccept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    updatePreview();
}

QString AddSiteDialog::prefix() const { return m_prefixEdit->text().trimmed(); }
QString AddSiteDialog::path() const   { return m_pathEdit->text(); }

void AddSiteDialog::onBrowse()
{
    QString start = getWebsitesDir();
    if (!QDir(start).exists())
        start = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);

    const QString dir = QFileDialog::getExistingDirectory(this, tr("Select Site Folder"), start);
    if (dir.isEmpty())
        return;

    m_pathEdit->setText(dir);

    if (!m_prefixManuallyEdited) {
        m_updatingPrefix = true;
        m_prefixEdit->setText(SiteManager::suggestPrefixFromPath(dir));
        m_updatingPrefix = false;
    }

    setError(QString());
    m_addButton->setEnabled(!m_pathEdit->text().isEmpty()
                            && !m_prefixEdit->text().trimmed().isEmpty());
    updatePreview();
}

void AddSiteDialog::onPrefixChanged(const QString& text)
{
    if (!m_updatingPrefix)
        m_prefixManuallyEdited = true;

    m_addButton->setEnabled(!m_pathEdit->text().isEmpty()
                            && !text.trimmed().isEmpty());
    setError(QString());
    updatePreview();
}

void AddSiteDialog::onAccept()
{
    QString err;
    if (!m_manager->addSite(prefix(), path(), &err)) {
        setError(err.isEmpty() ? tr("Failed to add site.") : err);
        return;
    }
    accept();
}

void AddSiteDialog::updatePreview()
{
    const QString p = SiteManager::normalizePrefix(m_prefixEdit->text());
    if (p.isEmpty())
        m_previewLabel->setText(tr("http://<onion>/"));
    else
        m_previewLabel->setText(QString("http://&lt;onion&gt;%1").arg(p.toHtmlEscaped()));
}

void AddSiteDialog::setError(const QString& msg)
{
    if (msg.isEmpty()) {
        m_errorLabel->hide();
        m_errorLabel->clear();
    } else {
        m_errorLabel->setText(msg);
        m_errorLabel->show();
    }
}
