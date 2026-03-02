#include "httpclientdialog.h"
#include<QRegularExpression>
#include "crypto.h"

HttpClientDialog::HttpClientDialog(QWidget *parent)
    : QDialog(parent)
{
    setupUI();
    setupConnections();
}

HttpClientDialog::~HttpClientDialog()
{
    // Cancel all active downloads on close
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

    // URL Bar
    QHBoxLayout *urlLayout = new QHBoxLayout();
    urlBar = new QLineEdit();
    urlBar->setPlaceholderText("Enter .onion URL or http://localhost:port");
    goButton = new QPushButton("Go");
    urlLayout->addWidget(urlBar);
    urlLayout->addWidget(goButton);

    // File List
    fileList = new QListWidget();
    fileList->setSelectionMode(QAbstractItemView::ExtendedSelection);
    QHBoxLayout *buttonLayout = new QHBoxLayout();
        resetButton = new QPushButton("Reset");
        downloadSelectedButton = new QPushButton("Download Selected");
        closeButton = new QPushButton("Close");
        buttonLayout->addWidget(resetButton);
        buttonLayout->addStretch(); // Push buttons to the right
        buttonLayout->addWidget(downloadSelectedButton);
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
}

void HttpClientDialog::fetchDirectoryListing()
{
    QString url = urlBar->text().trimmed();
    if (url.isEmpty()) {
        QMessageBox::warning(this, "Error", "Please enter a URL");
        return;
    }

    // Clear previous results
    fileList->clear();
    activeDownloads.clear();
    progressBars.clear();
    actionButtons.clear();

    // Fetch directory listing via curl
    QProcess *curlProcess = new QProcess(this);
    QStringList args = {
        //"--socks5-hostname", "127.0.0.1:9050",
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

    curlProcess->start("curl", args);
}

void HttpClientDialog::addFileToList(const QString& fileName, const QString& fileUrl)
{
    QListWidgetItem *item = new QListWidgetItem();

    // Create widget for file entry
    QWidget *widget = new QWidget();
    QHBoxLayout *layout = new QHBoxLayout(widget);
    layout->setContentsMargins(5, 2, 5, 2);

    // File name
    QLabel *nameLabel = new QLabel(fileName);
    nameLabel->setMinimumWidth(200);

    // Progress bar
    QProgressBar *progressBar = new QProgressBar();
    progressBar->setMinimum(0);
    progressBar->setMaximum(100);
    progressBar->setValue(0);
    progressBar->setVisible(false);

    // Download button
    QPushButton *actionButton = new QPushButton("Download");

    layout->addWidget(nameLabel);
    layout->addWidget(progressBar);
    layout->addWidget(actionButton);

    item->setSizeHint(widget->sizeHint());
    fileList->addItem(item);
    fileList->setItemWidget(item, widget);

    // Store references for later access
    progressBars[fileName] = progressBar;
    actionButtons[fileName] = actionButton;

    // Connect download button
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

    // Regex to capture href links
    QRegularExpression linkRegex("<a href=\"([^\"]+)\">([^<]+)</a>", QRegularExpression::CaseInsensitiveOption);
    QRegularExpressionMatchIterator matches = linkRegex.globalMatch(html);
    int count = 0;

    while (matches.hasNext()) {
        QRegularExpressionMatch match = matches.next();
        QString fileUrl = match.captured(1);
        QString fileName = match.captured(2);


        // Skip parent directory or non-files
        if (fileName == "../" || fileUrl.contains("://") || fileUrl.startsWith("?")) {
            continue;
        }

        // Keep leading slash for absolute paths; trim whitespace only
        fileUrl = fileUrl.trimmed();


        // Add to list
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

    // Extract onion host only (no path)
    QString onionHost;
    if (baseUrl.contains('/')) {
        onionHost = baseUrl.section('/', 0, 0); // e.g., "oyjms3dsazrshc5qkktjrzkhwzuhqcqaon5ahlmcsyr4cz47gfwx6pid.onion"
    } else {
        onionHost = baseUrl;
    }

    // Ensure fileUrl starts with single slash
    QString cleanedUrl = fileUrl;
    if (!cleanedUrl.startsWith('/')) {
        cleanedUrl = "/" + cleanedUrl;
    }

    QString fullUrl = onionHost + cleanedUrl;


    // Create downloads directory if it doesn't exist
    QString downloadDirpath = getDownloadsDirPath();
    QDir downloadsDir(downloadDirpath);
    if (!downloadsDir.exists()) {
        downloadsDir.mkpath(".");
    }

    QString filePath = downloadsDir.filePath(fileName);

    // Setup curl process
    QProcess *curlProcess = new QProcess(this);
    QStringList args = {
        "--socks5-hostname", bindIP,
        "--progress-bar",
        "--no-buffer",
        "--output", filePath,
        fullUrl
    };


    // Update UI
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

    curlProcess->start("curl", args);
}

void HttpClientDialog::updateProgress(const QString& fileName, const QByteArray& progressData)
{
    if (!progressBars.contains(fileName)) return;

    QProgressBar *progressBar = progressBars[fileName];
    QString progressStr = QString::fromUtf8(progressData);

    // Look for patterns like: "2.1%" - capture the number BEFORE the decimal
    QRegularExpression percentRegex(R"((\d+)\.\d%)");
    QRegularExpressionMatch match = percentRegex.match(progressStr);

    if (match.hasMatch()) {
        int percent = match.captured(1).toInt();
        if (percent > progressBar->value()) {
            progressBar->setValue(percent);
        }
    }

    // Also catch 100% case (no decimal)
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

        // Reset UI
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

            // MINIMAL ADDITION: Auto-decrypt .encbundle files
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
                            // Regular file download success
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


//AES


bool HttpClientDialog::decryptDownloadedFile(const QString &encryptedBundlePath, const QString &destPath)
{

    QFile file(encryptedBundlePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "❌ Failed to open bundle file";
        return false;
    }


    QDataStream in(&file);
    in.setVersion(QDataStream::Qt_6_0);

    // 1. Read encrypted AES key
    quint32 aesKeySize;
    in >> aesKeySize;

    QByteArray encryptedAESKey(aesKeySize, 0);
    int bytesRead1 = in.readRawData(encryptedAESKey.data(), aesKeySize);

    // 2. Read IV
    quint32 ivSize;
    in >> ivSize;

    QByteArray iv(ivSize, 0);
    int bytesRead2 = in.readRawData(iv.data(), ivSize);

    // 3. Read encrypted file data
    quint32 encryptedFileSize;
    in >> encryptedFileSize;

    QByteArray encryptedFileData(encryptedFileSize, 0);
    int bytesRead3 = in.readRawData(encryptedFileData.data(), encryptedFileSize);

    // Check if we read everything correctly
    if (bytesRead1 != aesKeySize || bytesRead2 != ivSize || bytesRead3 != encryptedFileSize) {
        qWarning() << "❌ Failed to read all data from bundle";
        return false;
    }


    // 4. Decrypt AES key using our private key (RSA decryption)
    QByteArray aesKey;
    if (!crypt.decrypt(encryptedAESKey, aesKey)) {
        qWarning() << "❌ Failed to decrypt AES key with RSA";
        return false;
    }

    // 5. Decrypt file data using AES decryption
    QByteArray decryptedFileData = crypt.decryptAES(encryptedFileData, aesKey, iv);
    if (decryptedFileData.isEmpty()) {
        qWarning() << "❌ Failed to decrypt file data with AES";
        return false;
    }

    // 6. Save decrypted file
    QFile outFile(destPath);
    if (!outFile.open(QIODevice::WriteOnly)) {
        qWarning() << "❌ Failed to open output file";
        return false;
    }
    qint64 bytesWritten = outFile.write(decryptedFileData);
    outFile.close();

    // 7. Clean up the encrypted bundle
    file.remove();

    return true;
}


void HttpClientDialog::resetAll()
{
    // Cancel all active downloads
    for (auto process : activeDownloads) {
        process->kill();
        process->deleteLater();
    }

    // Clear all containers
    activeDownloads.clear();
    progressBars.clear();
    actionButtons.clear();

    // Clear the file list widget
    fileList->clear();

    // Clear URL bar
    urlBar->clear();

    qDebug() << "All fields, downloads, and processes reset";
}

void HttpClientDialog::downloadSelectedFiles()
{
    QList<QListWidgetItem*> selectedItems = fileList->selectedItems();

    if (selectedItems.isEmpty()) {
        QMessageBox::information(this, "No Selection", "Please select files to download.");
        return;
    }

    // Download each selected file
    for (QListWidgetItem* item : selectedItems) {
        // Get the widget from the list item
        QWidget* widget = fileList->itemWidget(item);
        if (!widget) continue;

        // Find the download button in the widget
        QPushButton* downloadBtn = widget->findChild<QPushButton*>();
        if (downloadBtn && downloadBtn->isEnabled()) {
            // Simulate clicking the download button
            downloadBtn->click();
        }
    }

    QMessageBox::information(this, "Downloads Started",
                           QString("Started downloading %1 files").arg(selectedItems.size()));
}
