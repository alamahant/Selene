#pragma once

#include <QDialog>
#include<QTableWidget>
#include<optional>

class SiteManager;
struct Site;
class QPushButton;

class SiteManagerDialog : public QDialog
{
    Q_OBJECT
public:
    explicit SiteManagerDialog(SiteManager* manager, QWidget* parent = nullptr);

private slots:
    void refreshList();
    void onItemChanged(QTableWidgetItem* item);
    void onSelectionChanged();
    void onAdd();
    void onToggle();
    void onRemove();
    void onOpenFolder();
    void onRename();
    void onCopyUrl();
private:
    QString statusFor(const Site& s) const;
    QColor  statusColor(const Site& s) const;

    SiteManager* m_manager;
    QTableWidget* m_table;
    QPushButton* m_toggleBtn;
    QPushButton* m_removeBtn;
    QPushButton* m_openBtn;
    QPushButton* m_renameBtn;
    QPushButton* m_copyBtn = nullptr;
    bool m_refreshing = false;
    std::optional<Site> selectedSite() const;

};
