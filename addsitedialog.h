#pragma once

#include <QDialog>

class SiteManager;
class QLineEdit;
class QLabel;
class QPushButton;

class AddSiteDialog : public QDialog
{
    Q_OBJECT
public:
    explicit AddSiteDialog(SiteManager* manager, QWidget* parent = nullptr);

    QString prefix() const;
    QString path() const;

private slots:
    void onBrowse();
    void onPrefixChanged(const QString& text);
    void onAccept();

private:
    void updatePreview();
    void setError(const QString& msg);

    SiteManager* m_manager;
    QLineEdit* m_pathEdit;
    QLineEdit* m_prefixEdit;
    QLabel* m_previewLabel;
    QLabel* m_errorLabel;
    QPushButton* m_addButton;
    bool m_prefixManuallyEdited = false;
    bool m_updatingPrefix = false;
};