#include "httpclientdialog.h"
#include<QRegularExpression>
#include "crypto.h"
#include<QApplication>
#include"constants.h"
#include<QDesktopServices>

HttpClientDialog::HttpClientDialog(SiteManager *siteManager, QWidget *parent)
    : QDialog(parent)
    , m_siteManager(siteManager)
{
    setupUI();
    setupConnections();
}

HttpClientDialog::~HttpClientDialog()
{
    for (auto process : activeDownloads) {
        process->kill();
        process->deleteLater();
    }
}

void HttpClientDialog::setupUI()
{
    setWindowTitle("HTTP Client");
    setMinimumSize(600, 400);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    QHBoxLayout *urlLayout = new QHBoxLayout();
    urlBar = new QLineEdit();
    urlBar->setPlaceholderText("Enter .onion URL or http://localhost:port");
    goButton = new QPushButton("Go");
    urlLayout->addWidget(urlBar);
    urlLayout->addWidget(goButton);

    fileList = new QListWidget();
    fileList->setSelectionMode(QAbstractItemView::ExtendedSelection);
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    resetButton = new QPushButton("Reset");
    downloadSelectedButton = new QPushButton("Download Selected");
    openDownloadButton = new QPushButton("Downloads", this);
    closeButton = new QPushButton("Close");
    buttonLayout->addWidget(resetButton);
    buttonLayout->addStretch(); // Push buttons to the right
    buttonLayout->addWidget(downloadSelectedButton);
    buttonLayout->addWidget(openDownloadButton);
    buttonLayout->addStretch(); // Push buttons to the right

    buttonLayout->addWidget(closeButton);

    mainLayout->addLayout(urlLayout);
    mainLayout->addWidget(fileList);
    mainLayout->addLayout(buttonLayout); // Use the button layout
}

void HttpClientDialog::setupConnections()
{
    connect(goButton, &QPushButton::clicked, this, &HttpClientDialog::fetchDirectoryListing);
    connect(urlBar, &QLineEdit::returnPressed, this, &HttpClientDialog::fetchDirectoryListing);
    connect(closeButton, &QPushButton::clicked, this, &HttpClientDialog::close);
    connect(resetButton, &QPushButton::clicked, this, &HttpClientDialog::resetAll);
    connect(downloadSelectedButton, &QPushButton::clicked, this, &HttpClientDialog::downloadSelectedFiles);
    connect(openDownloadButton, &QPushButton::clicked, this, [this]{
        QString path = getDownloadsDirPath();
        QDesktopServices::openUrl(QUrl::fromLocalFile(path));
    });
}

void HttpClientDialog::fetchDirectoryListing()
{
    QString url = urlBar->text().trimmed();
    if (url.isEmpty()) {
        QMessageBox::warning(this, "Error", "Please enter a URL");
        return;
    }

    fileList->clear();
    activeDownloads.clear();
    progressBars.clear();
    actionButtons.clear();

    QUrl u(url);

    QString p = url;
    int slash = p.indexOf('/');
    if (slash >= 0)
        p = p.mid(slash);
    else
        p = "/";

    if (m_siteManager) {

        Site s;
        if (m_siteManager->match(p, s) && s.enabled) {
            QMessageBox::information(this, "Website",
                "This URL is a website served by Selene. "
                "Open it in Tor Browser to view it. The HTTP Client "
                "only browses file shares.");
            return;
        }
    }

    QProcess *curlProcess = new QProcess(this);
    QStringList args = {
        "--socks5-hostname", bindIP,

        "--silent",
        url
    };

    connect(curlProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            [this, curlProcess](int exitCode, QProcess::ExitStatus exitStatus) {
        if (exitCode == 0 && exitStatus == QProcess::NormalExit) {
            QString html = QString::fromUtf8(curlProcess->readAllStandardOutput());
            parseHtmlListing(html);
        } else {
            QMessageBox::critical(this, "Error", "Failed to fetch directory listing");
        }
        curlProcess->deleteLater();
    });

#ifdef Q_OS_WIN
    const QString curlExe = QApplication::applicationDirPath() + "/curl/curl.exe";
    const QString caBundle = QApplication::applicationDirPath() + "/curl/curl-ca-bundle.crt";

    if (!QFile::exists(curlExe)) {
        QMessageBox::critical(this, "Error",
            QString("curl not found at:\n%1").arg(curlExe));
        curlProcess->deleteLater();
        return;
    }

    if (QFile::exists(caBundle)) {
        args << "--cacert" << caBundle;

        QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
        env.insert("CURL_CA_BUNDLE", caBundle);
        curlProcess->setProcessEnvironment(env);
    }
#else
    const QString curlExe = "curl";
#endif

    curlProcess->start(curlExe, args);
}

void HttpClientDialog::addFileToList(const QString& fileName, const QString& fileUrl)
{
    QListWidgetItem *item = new QListWidgetItem();

    QWidget *widget = new QWidget();
    QHBoxLayout *layout = new QHBoxLayout(widget);
    layout->setContentsMargins(5, 2, 5, 2);

    QLabel *nameLabel = new QLabel(fileName);
    nameLabel->setMinimumWidth(200);

    QProgressBar *progressBar = new QProgressBar();
    progressBar->setMinimum(0);
    progressBar->setMaximum(100);
    progressBar->setValue(0);
    progressBar->setVisible(false);

    QPushButton *actionButton = new QPushButton("Download");

    layout->addWidget(nameLabel);
    layout->addWidget(progressBar);
    layout->addWidget(actionButton);

    item->setSizeHint(widget->sizeHint());
    fileList->addItem(item);
    fileList->setItemWidget(item, widget);

    progressBars[fileName] = progressBar;
    actionButtons[fileName] = actionButton;

    connect(actionButton, &QPushButton::clicked, this, [this, fileName, fileUrl]() {
        if (activeDownloads.contains(fileName)) {
            cancelDownload(fileName);
        } else {
            startDownload(fileName, fileUrl);
        }
    });
}

void HttpClientDialog::parseHtmlListing(const QString& html)
{

    QRegularExpression linkRegex("<a href=\"([^\"]+)\">([^<]+)</a>", QRegularExpression::CaseInsensitiveOption);
    QRegularExpressionMatchIterator matches = linkRegex.globalMatch(html);
    int count = 0;

    while (matches.hasNext()) {
        QRegularExpressionMatch match = matches.next();
        QString fileUrl = match.captured(1);
        QString fileName = match.captured(2);


        if (fileName == "../" || fileUrl.contains("://") || fileUrl.startsWith("?")) {
            continue;
        }

        fileUrl = fileUrl.trimmed();


        addFileToList(fileName, fileUrl);
        count++;
    }


    if (count == 0) {
        QListWidgetItem *item = new QListWidgetItem("No files found or unable to parse directory listing");
        fileList->addItem(item);
    }
}


void HttpClientDialog::startDownload(const QString& fileName, const QString& fileUrl)
{

    QString baseUrl = urlBar->text().trimmed();

    if (baseUrl.isEmpty()) {
        QMessageBox::warning(this, "Error", "Base URL is empty");
        return;
    }

    QString onionHost;
    if (baseUrl.contains('/')) {
        onionHost = baseUrl.section('/', 0, 0); // e.g., "oyjms3dsazrshc5qkktjrzkhwzuhqcqaon5ahlmcsyr4cz47gfwx6pid.onion"
    } else {
        onionHost = baseUrl;
    }

    QString cleanedUrl = fileUrl;
    if (!cleanedUrl.startsWith('/')) {
        cleanedUrl = "/" + cleanedUrl;
    }

    QString fullUrl = onionHost + cleanedUrl;


    QString downloadDirpath = getDownloadsDirPath();
    QDir downloadsDir(downloadDirpath);
    if (!downloadsDir.exists()) {
        downloadsDir.mkpath(".");
    }

    QString filePath = downloadsDir.filePath(fileName);

    QProcess *curlProcess = new QProcess(this);
    QStringList args = {
        "--socks5-hostname", bindIP,
        "--progress-bar",
        "--no-buffer",
        "--output", filePath,
        fullUrl
    };


    QProgressBar *progressBar = progressBars[fileName];
    QPushButton *actionButton = actionButtons[fileName];
    progressBar->setVisible(true);
    progressBar->setValue(0);
    actionButton->setText("Cancel");

    activeDownloads[fileName] = curlProcess;

    connect(curlProcess, &QProcess::readyReadStandardError, this, [this, fileName, curlProcess]() {
        updateProgress(fileName, curlProcess->readAllStandardError());
    });

    connect(curlProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            [this, fileName](int exitCode, QProcess::ExitStatus exitStatus) {
        downloadFinished(fileName, exitCode, exitStatus);
    });

#ifdef Q_OS_WIN
    const QString curlExe = "curl.exe";
#else
    const QString curlExe = "curl";
#endif
    curlProcess->start(curlExe, args);
}

void HttpClientDialog::updateProgress(const QString& fileName, const QByteArray& progressData)
{
    if (!progressBars.contains(fileName)) return;

    QProgressBar *progressBar = progressBars[fileName];
    QString progressStr = QString::fromUtf8(progressData);

    QRegularExpression percentRegex(R"((\d+)\.\d%)");
    QRegularExpressionMatch match = percentRegex.match(progressStr);

    if (match.hasMatch()) {
        int percent = match.captured(1).toInt();
        if (percent > progressBar->value()) {
            progressBar->setValue(percent);
        }
    }

    QRegularExpression hundredRegex(R"(\b(100)%\b)");
    QRegularExpressionMatch hundredMatch = hundredRegex.match(progressStr);
    if (hundredMatch.hasMatch()) {
        progressBar->setValue(100);
    }
}

void HttpClientDialog::cancelDownload(const QString& fileName)
{
    if (activeDownloads.contains(fileName)) {
        QProcess *process = activeDownloads[fileName];
        process->kill();
        process->deleteLater();
        activeDownloads.remove(fileName);

        QProgressBar *progressBar = progressBars[fileName];
        QPushButton *actionButton = actionButtons[fileName];

        progressBar->setVisible(false);
        actionButton->setText("Download");
    }
}


void HttpClientDialog::downloadFinished(const QString& fileName, int exitCode, QProcess::ExitStatus exitStatus)
{
    if (activeDownloads.contains(fileName)) {
        activeDownloads[fileName]->deleteLater();
        activeDownloads.remove(fileName);
    }

    if (progressBars.contains(fileName) && actionButtons.contains(fileName)) {
        QProgressBar *progressBar = progressBars[fileName];
        QPushButton *actionButton = actionButtons[fileName];

        if (exitCode == 0 && exitStatus == QProcess::NormalExit) {
            progressBar->setValue(100);
            actionButton->setText("Done");
            actionButton->setEnabled(false);

            if (fileName.endsWith(".encbundle")) {
                QString downloadDir = getDownloadsDirPath();
                QString encryptedPath = downloadDir + QDir::separator() + fileName;
                QString decryptedPath = encryptedPath;
                decryptedPath.replace(".encbundle", ""); // Remove .encbundle extension

                if (decryptDownloadedFile(encryptedPath, decryptedPath)) {
                    QMessageBox::information(this, "File Decrypted",
                                           QString("File automatically decrypted to:\n%1").arg(decryptedPath));
                } else {
                    QMessageBox::warning(this, "Decryption Failed",
                                       QString("Failed to decrypt file:\n%1").arg(encryptedPath));
                }
            } else {
                            QMessageBox::information(this, "Download Complete",
                                                   QString("'%1' downloaded to:\n%2")
                                                     .arg(fileName)

                                                     .arg(getDownloadsDirPath()));
                        }

        } else {
            progressBar->setVisible(false);
            actionButton->setText("Download");
            QMessageBox::warning(this, "Download Failed",
                               QString("Failed to download '%1'").arg(fileName));
        }
    }
}




bool HttpClientDialog::decryptDownloadedFile(const QString &encryptedBundlePath, const QString &destPath)
{

    QFile file(encryptedBundlePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "❌ Failed to open bundle file";
        return false;
    }


    QDataStream in(&file);
    in.setVersion(QDataStream::Qt_6_0);

    quint32 aesKeySize;
    in >> aesKeySize;

    QByteArray encryptedAESKey(aesKeySize, 0);
    int bytesRead1 = in.readRawData(encryptedAESKey.data(), aesKeySize);

    quint32 ivSize;
    in >> ivSize;

    QByteArray iv(ivSize, 0);
    int bytesRead2 = in.readRawData(iv.data(), ivSize);

    quint32 encryptedFileSize;
    in >> encryptedFileSize;

    QByteArray encryptedFileData(encryptedFileSize, 0);
    int bytesRead3 = in.readRawData(encryptedFileData.data(), encryptedFileSize);

    if (bytesRead1 != aesKeySize || bytesRead2 != ivSize || bytesRead3 != encryptedFileSize) {
        qWarning() << "❌ Failed to read all data from bundle";
        return false;
    }


    QByteArray aesKey;
    if (!crypt.decrypt(encryptedAESKey, aesKey)) {
        qWarning() << "❌ Failed to decrypt AES key with RSA";
        return false;
    }

    QByteArray decryptedFileData = crypt.decryptAES(encryptedFileData, aesKey, iv);
    if (decryptedFileData.isEmpty()) {
        qWarning() << "❌ Failed to decrypt file data with AES";
        return false;
    }

    QFile outFile(destPath);
    if (!outFile.open(QIODevice::WriteOnly)) {
        qWarning() << "❌ Failed to open output file";
        return false;
    }
    qint64 bytesWritten = outFile.write(decryptedFileData);
    outFile.close();

    file.remove();

    return true;
}


void HttpClientDialog::resetAll()
{
    for (auto process : activeDownloads) {
        process->kill();
        process->deleteLater();
    }

    activeDownloads.clear();
    progressBars.clear();
    actionButtons.clear();

    fileList->clear();

    urlBar->clear();

}

void HttpClientDialog::downloadSelectedFiles()
{
    QList<QListWidgetItem*> selectedItems = fileList->selectedItems();

    if (selectedItems.isEmpty()) {
        QMessageBox::information(this, "No Selection", "Please select files to download.");
        return;
    }

    for (QListWidgetItem* item : selectedItems) {
        QWidget* widget = fileList->itemWidget(item);
        if (!widget) continue;

        QPushButton* downloadBtn = widget->findChild<QPushButton*>();
        if (downloadBtn && downloadBtn->isEnabled()) {
            downloadBtn->click();
        }
    }

    QMessageBox::information(this, "Downloads Started",
                           QString("Started downloading %1 files").arg(selectedItems.size()));
}
