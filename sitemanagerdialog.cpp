#include "sitemanagerdialog.h"
#include "sitemanager.h"
#include "addsitedialog.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTableWidget>
#include <QHeaderView>
#include <QPushButton>
#include <QLabel>
#include <QDialogButtonBox>
#include <QMessageBox>
#include <QInputDialog>
#include <QDesktopServices>
#include <QUrl>
#include <QFileInfo>
#include <QDir>
#include <QStandardPaths>
#include <QSignalBlocker>
#include"constants.h"
#include<QApplication>
#include<QClipboard>

SiteManagerDialog::SiteManagerDialog(SiteManager* manager, QWidget* parent)
    : QDialog(parent), m_manager(manager)
{
    setWindowTitle(tr("Website Manager"));
    resize(760, 440);

    auto* layout = new QVBoxLayout(this);

    QString sitesDir = getWebsitesDir();

    auto* infoRow = new QHBoxLayout();
    auto* info = new QLabel(tr("Sites are served from %1").arg(QDir::toNativeSeparators(sitesDir)), this);
    info->setStyleSheet("color: #666;");
    infoRow->addWidget(info);
    infoRow->addStretch();

    QPushButton *infoButton = new QPushButton("ⓘ", this);
    infoButton->setObjectName("infoButton");
    infoButton->setFixedSize(24, 24);
    QString tip = QString(
        "<html><body style='white-space: nowrap;'>"
        "<b>Selene — Website Hosting Guide</b><br><br>"

        "<b>What this does</b><br>"
        "• Serves static sites over your existing HTTP onion<br>"
        "• Runs alongside the file share, not instead of it<br>"
        "• Same onion address, different paths<br><br>"

        "<b>Where sites live</b><br>"
        "• Folder: %1<br>"
        "• One folder per site (e.g. <i>demo/</i>, <i>blog/</i>)<br>"
        "• The folder name suggests the URL prefix<br><br>"

        "<b>Adding a site</b><br>"
        "• Create a folder under the sites directory<br>"
        "• Put an <b>index.html</b> inside it<br>"
        "• Click <b>Add Site…</b> → pick the folder<br>"
        "• Prefix is prefilled (<i>/folder/</i>), edit if you want<br>"
        "• The dialog shows the URL that will be served<br><br>"

        "<b>What gets served</b><br>"
        "• HTML, CSS, JS, images, fonts, audio, video — rendered inline<br>"
        "• PDFs open in the browser's viewer<br>"
        "• Archives (zip, tar, gz, 7z, rar) and binaries — downloaded<br>"
        "• Unknown types are downloaded<br>"
        "• No PHP, no server-side code: static only<br><br>"

        "<b>URLs and links</b><br>"
        "• Home: <i>http://&lt;onion&gt;/prefix/</i><br>"
        "• Visiting <i>/prefix</i> redirects to <i>/prefix/</i><br>"
        "• Relative links (href=\"about.html\") work as on any site<br>"
        "• Subfolders with their own index.html work<br>"
        "• Folder without index.html returns 404 (no listing)<br><br>"

        "<b>Managing sites</b><br>"
        "• <b>Add Site…</b> — register a new folder<br>"
        "• <b>Enable / Disable</b> — toggle without deleting<br>"
        "• <b>Remove</b> — unregister only; files stay on disk<br>"
        "• <b>Open Folder</b> — open the site folder in your file manager<br>"
        "• <b>Rename…</b> — change the URL prefix<br>"
        "• <b>Copy URL</b> — copy the full onion URL to clipboard<br><br>"

        "<b>Status column</b><br>"
        "• <b>OK</b> — enabled, folder exists, index.html present<br>"
        "• <b>No index.html</b> — page will 404 until added<br>"
        "• <b>Path missing</b> — folder moved or deleted<br>"
        "• <b>disabled</b> — registered but not served<br><br>"

        "<b>File share vs website</b><br>"
        "• <i>/</i> — file share (directory listing, all downloads)<br>"
        "• <i>/prefix/</i> — website (pages rendered)<br>"
        "• The file share is unchanged by adding sites<br><br>"

        "<b>Notes</b><br>"
        "• Sites survive restart (stored in QSettings)<br>"
        "• Adding or removing sites does not restart Tor<br>"
        "• After starting Selene, wait ~30s for Tor to publish<br>"
        "• If a site 404s, check the Status column first"
        "</body></html>").arg(QDir::toNativeSeparators(sitesDir));

    infoButton->setToolTip(tip);

    infoButton->setStyleSheet(
        "QPushButton#infoButton { "
        "background-color: rgba(37, 99, 235, 0.15); "
        "border-radius: 16px; "
        "font-size: 18px; "
        "font-weight: bold; "
        "color: #60a5fa; "
        "border: none; "
        "padding: 0px; "
        "} "
        "QPushButton#infoButton:hover { "
        "background-color: rgba(37, 99, 235, 0.3); "
        "color: #93bbfc; "
        "}"
    );
    infoRow->addWidget(infoButton);

    layout->addLayout(infoRow);

    m_table = new QTableWidget(this);
    m_table->setColumnCount(4);
    m_table->setHorizontalHeaderLabels({tr("Enabled"), tr("Prefix"), tr("Path"), tr("Status")});
    m_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    m_table->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    m_table->verticalHeader()->setVisible(false);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    layout->addWidget(m_table, 1);

    auto* buttons = new QHBoxLayout();
    auto* addBtn = new QPushButton(tr("Add Site…"), this);
    m_toggleBtn = new QPushButton(tr("Enable"), this);
    m_removeBtn = new QPushButton(tr("Remove"), this);
    m_openBtn   = new QPushButton(tr("Open Folder"), this);
    m_copyBtn = new QPushButton(tr("Copy URL"), this);
    m_renameBtn = new QPushButton(tr("Rename…"), this);

    buttons->addWidget(addBtn);
    buttons->addWidget(m_toggleBtn);
    buttons->addWidget(m_removeBtn);
    buttons->addWidget(m_openBtn);
    buttons->addWidget(m_copyBtn);

    buttons->addWidget(m_renameBtn);
    buttons->addStretch();
    layout->addLayout(buttons);

    auto* box = new QDialogButtonBox(QDialogButtonBox::Close, this);
    layout->addWidget(box);
    connect(box, &QDialogButtonBox::rejected, this, &QDialog::reject);

    connect(addBtn, &QPushButton::clicked, this, &SiteManagerDialog::onAdd);
    connect(m_toggleBtn, &QPushButton::clicked, this, &SiteManagerDialog::onToggle);
    connect(m_removeBtn, &QPushButton::clicked, this, &SiteManagerDialog::onRemove);
    connect(m_openBtn, &QPushButton::clicked, this, &SiteManagerDialog::onOpenFolder);
    connect(m_copyBtn, &QPushButton::clicked, this, &SiteManagerDialog::onCopyUrl);
    connect(m_renameBtn, &QPushButton::clicked, this, &SiteManagerDialog::onRename);

    connect(m_table, &QTableWidget::itemChanged, this, &SiteManagerDialog::onItemChanged);
    connect(m_table, &QTableWidget::itemSelectionChanged, this, &SiteManagerDialog::onSelectionChanged);

    connect(m_manager, &SiteManager::sitesChanged, this, &SiteManagerDialog::refreshList);

    refreshList();
}

void SiteManagerDialog::refreshList()
{
    m_refreshing = true;
    QSignalBlocker blocker(m_table);

    const QString selectedPrefix = selectedSite() ? selectedSite()->prefix : QString();

    m_table->setRowCount(0);
    const QList<Site> list = m_manager->sites();

    for (const Site& s : list) {
        const int row = m_table->rowCount();
        m_table->insertRow(row);

        auto* check = new QTableWidgetItem();
        check->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        check->setCheckState(s.enabled ? Qt::Checked : Qt::Unchecked);
        check->setData(Qt::UserRole, s.prefix);
        m_table->setItem(row, 0, check);

        auto* prefixItem = new QTableWidgetItem(s.prefix);
        auto* pathItem   = new QTableWidgetItem(QDir::toNativeSeparators(s.path));
        auto* statusItem = new QTableWidgetItem(statusFor(s));
        statusItem->setForeground(statusColor(s));

        if (!s.enabled) {
            prefixItem->setForeground(QBrush(Qt::gray));
            pathItem->setForeground(QBrush(Qt::gray));
        }

        m_table->setItem(row, 1, prefixItem);
        m_table->setItem(row, 2, pathItem);
        m_table->setItem(row, 3, statusItem);

        if (s.prefix == selectedPrefix)
            m_table->selectRow(row);
    }

    m_refreshing = false;
    onSelectionChanged();
}

QString SiteManagerDialog::statusFor(const Site& s) const
{
    if (!s.enabled)
        return tr("disabled");
    QFileInfo fi(s.path);
    if (!fi.exists() || !fi.isDir())
        return tr("⚠ Path missing");
    QFileInfo index(QDir(s.path).filePath("index.html"));
    if (!index.exists() || !index.isFile())
        return tr("⚠ No index.html");
    return tr("OK");
}

QColor SiteManagerDialog::statusColor(const Site& s) const
{
    if (!s.enabled) return Qt::gray;
    QFileInfo fi(s.path);
    if (!fi.exists() || !fi.isDir()) return QColor("#c0392b");
    QFileInfo index(QDir(s.path).filePath("index.html"));
    if (!index.exists() || !index.isFile()) return QColor("#e67e22");
    return QColor("#27ae60");
}

void SiteManagerDialog::onItemChanged(QTableWidgetItem* item)
{
    if (m_refreshing || item->column() != 0)
        return;
    const QString prefix = item->data(Qt::UserRole).toString();
    const bool enabled = (item->checkState() == Qt::Checked);
    m_manager->setEnabled(prefix, enabled);
}
/*
const Site* SiteManagerDialog::selectedSite() const
{
    const auto rows = m_table->selectionModel()->selectedRows();
    if (rows.isEmpty())
        return nullptr;
    const int row = rows.first().row();
    QTableWidgetItem* item = m_table->item(row, 0);
    if (!item) return nullptr;
    const QString prefix = item->data(Qt::UserRole).toString();
    for (const Site& s : m_manager->sites())
        if (s.prefix == prefix)
            return &s;   // NOTE: see caveat below
    return nullptr;
}
*/


void SiteManagerDialog::onSelectionChanged()
{
    auto s = selectedSite();
    const bool has = s.has_value();
    m_toggleBtn->setEnabled(has);
    m_removeBtn->setEnabled(has);
    m_openBtn->setEnabled(has);
    m_renameBtn->setEnabled(has);
    m_toggleBtn->setText(has && s->enabled ? tr("Disable") : tr("Enable"));
}

void SiteManagerDialog::onAdd()
{
    AddSiteDialog dlg(m_manager, this);
    dlg.exec();
}

void SiteManagerDialog::onToggle()
{
    auto s = selectedSite();
    if (!s) return;
    m_manager->setEnabled(s->prefix, !s->enabled);
}

void SiteManagerDialog::onRemove()
{
    auto s = selectedSite();
    if (!s) return;
    const auto ret = QMessageBox::question(this, tr("Remove Site"),
        tr("Remove site %1?\n\nThe folder and its files will NOT be deleted.").arg(s->prefix));
    if (ret != QMessageBox::Yes)
        return;
    m_manager->removeSite(s->prefix);
}

void SiteManagerDialog::onOpenFolder()
{
    auto s = selectedSite();
    if (!s) return;
    QDesktopServices::openUrl(QUrl::fromLocalFile(s->path));
}

void SiteManagerDialog::onRename()
{
    auto s = selectedSite();
    if (!s) return;
    bool ok = false;
    const QString newPrefix = QInputDialog::getText(this, tr("Rename Site"),
        tr("New prefix:"), QLineEdit::Normal, s->prefix, &ok);
    if (!ok || newPrefix.trimmed().isEmpty())
        return;
    QString err;
    if (!m_manager->renamePrefix(s->prefix, newPrefix, &err))
        QMessageBox::warning(this, tr("Rename failed"), err);
}

void SiteManagerDialog::onCopyUrl()
{
    auto s = selectedSite();
    if (!s) return;

    QString url = QString("http://%1%2").arg(httpOnionAddress, s->prefix);

    QApplication::clipboard()->setText(url);

    QMessageBox::information(this, tr("Copied"),
        tr("Copied to clipboard:\n%1").arg(url));
}

std::optional<Site> SiteManagerDialog::selectedSite() const
{
    const auto rows = m_table->selectionModel()->selectedRows();
    if (rows.isEmpty()) return std::nullopt;
    const int row = rows.first().row();
    QTableWidgetItem* item = m_table->item(row, 0);
    if (!item) return std::nullopt;
    const QString prefix = item->data(Qt::UserRole).toString();
    for (const Site& s : m_manager->sites())
        if (s.prefix == prefix)
            return s;
    return std::nullopt;
}
