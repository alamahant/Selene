#include "logviewerdialog.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTextEdit>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QFile>
#include <QTextStream>
#include <QMessageBox>
#include <QRegularExpression>

LogViewerDialog::LogViewerDialog(const QString& logPath, QWidget* parent)
    : QDialog(parent), m_logPath(logPath)
{
    setWindowTitle("Chat Log Viewer");
    setMinimumSize(700, 500);

    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // Search bar
    QHBoxLayout* searchLayout = new QHBoxLayout();
    QLabel* searchLabel = new QLabel("Search Pattern:", this);
    m_searchEdit = new QLineEdit(this);
    QPushButton* searchBtn = new QPushButton("Search", this);
    searchLayout->addWidget(searchLabel);
    searchLayout->addWidget(m_searchEdit);
    searchLayout->addWidget(searchBtn);

    // Log display
    m_logEdit = new QTextEdit(this);
    m_logEdit->setReadOnly(true);

    // Truncate button
    QPushButton* truncateBtn = new QPushButton("Truncate Log", this);

    mainLayout->addLayout(searchLayout);
    mainLayout->addWidget(m_logEdit);
    mainLayout->addWidget(truncateBtn);

    connect(searchBtn, &QPushButton::clicked, this, &LogViewerDialog::onSearchClicked);
    connect(truncateBtn, &QPushButton::clicked, this, &LogViewerDialog::onTruncateClicked);

    loadLog();
}

void LogViewerDialog::onSearchClicked() {
    QString pattern = m_searchEdit->text();
    if (pattern.isEmpty()) {
        m_logEdit->setPlainText(m_logContents);
        return;
    }
    // Simple regex search, highlight matches
    QRegularExpression re(pattern, QRegularExpression::CaseInsensitiveOption);
    QStringList lines = m_logContents.split('\n');
    QStringList matched;
    for (const QString& line : lines) {
        if (line.contains(re)) {
            matched << line;
        }
    }
    m_logEdit->setPlainText(matched.join('\n'));
}

void LogViewerDialog::onTruncateClicked() {
    if (QMessageBox::question(this, "Truncate Log", "Are you sure you want to truncate the log file?") == QMessageBox::Yes) {
        QFile file(m_logPath);
        if (file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            file.close();
            m_logContents.clear();
            m_logEdit->clear();
            QMessageBox::information(this, "Truncate Log", "Log file truncated.");
        } else {
            QMessageBox::warning(this, "Truncate Log", "Failed to truncate log file.");
        }
    }
}

void LogViewerDialog::loadLog() {
    QFile file(m_logPath);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&file);
        m_logContents = in.readAll();
        m_logEdit->setPlainText(m_logContents);
        file.close();
    } else {
        m_logEdit->setPlainText("Unable to open log file: " + m_logPath);
    }
}
