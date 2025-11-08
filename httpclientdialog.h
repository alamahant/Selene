#ifndef HTTPCLIENTDIALOG_H
#define HTTPCLIENTDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QPushButton>
#include <QListWidget>
#include <QListWidgetItem>
#include <QProgressBar>
#include <QProcess>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QRegularExpression>
#include <QMessageBox>
#include <QDir>
#include <QStandardPaths>
#include <QHash>
#include"constants.h"

class HttpClientDialog : public QDialog
{
    Q_OBJECT

public:
    explicit HttpClientDialog(QWidget *parent = nullptr);
    ~HttpClientDialog();

private slots:
    void fetchDirectoryListing();
    void parseHtmlListing(const QString& html);
    void startDownload(const QString& fileName, const QString& fileUrl);
    void cancelDownload(const QString& fileName);
    void updateProgress(const QString& fileName, const QByteArray& progressData);
    void downloadFinished(const QString& fileName, int exitCode, QProcess::ExitStatus exitStatus);
    void resetAll();
    void downloadSelectedFiles();

private:
    void setupUI();
    void setupConnections();
    void addFileToList(const QString& fileName, const QString& fileUrl);

    // UI Elements
    QLineEdit *urlBar;
    QPushButton *goButton;
    QListWidget *fileList;
    QPushButton *closeButton;

    // Download Management
    QHash<QString, QProcess*> activeDownloads;
    QHash<QString, QProgressBar*> progressBars;
    QHash<QString, QPushButton*> actionButtons;
    QString bindIP = "127.0.0.1:" + QString::number(DefaultProxyPort);
    bool decryptDownloadedFile(const QString &encryptedBundlePath, const QString &destPath);
    QPushButton *resetButton;
    QPushButton *downloadSelectedButton;
};
#endif // HTTPCLIENTDIALOG_H
