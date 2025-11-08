#include "bridgemanagerdialog.h"


#include <QRegularExpression>
#include<QDir>

BridgeManagerDialog::BridgeManagerDialog(QWidget *parent)
    : QDialog(parent)
{
    setupUI();
    setupConnections();

    // RESTORE SETTINGS AND SET CHECKBOX STATE ← NEW
        if (settings.contains("bridges/enabled")) {
            bool bridgesEnabled = settings.value("bridges/enabled").toBool();
            enableBridgesCheckbox->setChecked(bridgesEnabled);
        }

    loadCurrentBridges();
}

void BridgeManagerDialog::setupUI()
{
    setWindowTitle("Tor Bridge Manager");
    setWindowIcon(QIcon(":/icons/selene.png"));
    setMinimumSize(600, 450);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    // Instructions
    QLabel *instructions = new QLabel(
        "Paste bridge lines below (one bridge per line).\n"
        "Supported Bridge Types: obfs4 | webtunnel\n"
        "Examples:\n"
        "• obfs4 192.95.36.142:443 C6A6B... cert=XYZ... iat-mode=0\n"
        "• webtunnel 198.251.81.150:443 A1B2C... url=https://example.com/\n"
        "• Or just paste the full 'Bridge <type>...' line\n"
        "NOTE: Restart Selene for changes to take effect!"
    );
    instructions->setWordWrap(true);

    // Bridge text area
    bridgeTextEdit = new QTextEdit();
    bridgeTextEdit->setPlaceholderText("Paste bridge lines here...");

    // Status label
    statusLabel = new QLabel("Ready");
    statusLabel->setStyleSheet("QLabel { color: blue; }");

    // Buttons
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    addButton = new QPushButton("Add Bridges to Torrc");
    clearButton = new QPushButton("Clear Input");
    closeButton = new QPushButton("Close");
    deleteButton = new QPushButton("Delete All Bridges");

    enableBridgesCheckbox = new QCheckBox("Enable Bridges");
    enableBridgesCheckbox->setChecked(true);

    buttonLayout->addWidget(addButton);
    buttonLayout->addWidget(clearButton);
    buttonLayout->addWidget(deleteButton);
    buttonLayout->addStretch();
    buttonLayout->addWidget(closeButton);

    // Assemble layout
    mainLayout->addWidget(instructions);
    mainLayout->addWidget(enableBridgesCheckbox);  // ← NEW
    mainLayout->addWidget(bridgeTextEdit);
    mainLayout->addWidget(statusLabel);
    mainLayout->addLayout(buttonLayout);
}

void BridgeManagerDialog::setupConnections()
{
    connect(addButton, &QPushButton::clicked, this, &BridgeManagerDialog::addBridges);
    connect(clearButton, &QPushButton::clicked, this, [this]() {
        bridgeTextEdit->clear();
        statusLabel->setText("Input cleared");
    });
    connect(closeButton, &QPushButton::clicked, this, &QDialog::close);
    connect(deleteButton, &QPushButton::clicked, this, &BridgeManagerDialog::deleteAllBridges);
    connect(enableBridgesCheckbox, &QCheckBox::toggled, this, &BridgeManagerDialog::toggleBridges);
}

void BridgeManagerDialog::addBridges()
{
    QString input = bridgeTextEdit->toPlainText().trimmed();
    if (input.isEmpty()) {
        QMessageBox::warning(this, "No Input", "Please paste some bridge lines first.");
        return;
    }

    QStringList bridges;
    QStringList lines = input.split('\n', Qt::SkipEmptyParts);

    for (QString line : lines) {
        line = line.trimmed();

        // Skip empty lines
        if (line.isEmpty()) continue;

        // If line doesn't start with "Bridge", prepend it
        if (!line.startsWith("Bridge ", Qt::CaseInsensitive)) {
            line = "Bridge " + line;
        }

        bridges << line;
    }

    if (bridges.isEmpty()) {
        QMessageBox::warning(this, "No Valid Bridges", "No valid bridge lines found.");
        return;
    }

    // Add to torrc
    //QFile torrcFile(getTorrcPath());
    QFile torrcFile(torrcPath);

    if (!torrcFile.open(QIODevice::Append | QIODevice::Text)) {
        QMessageBox::critical(this, "Error", "Cannot open torrc file for writing.");
        return;
    }

    QTextStream stream(&torrcFile);
    stream << "\n# Bridges added by Selene - " << QDateTime::currentDateTime().toString() << "\n";
    for (const QString &bridge : bridges) {
        stream << bridge << "\n";
    }
    //stream << "# End of bridges\n";
    torrcFile.close();

    statusLabel->setText(QString("Added %1 bridges to torrc. Restart Tor to apply.").arg(bridges.size()));
    QMessageBox::information(this, "Success",
        QString("Added %1 bridges to torrc.\n\nYou need to restart Tor for changes to take effect.").arg(bridges.size()));
}

void BridgeManagerDialog::clearBridges()
{
    // Optional: Add functionality to remove bridges from torrc
    QMessageBox::information(this, "Clear Bridges",
        "To remove bridges, edit torrc file manually or use the reset function.");
}

void BridgeManagerDialog::loadCurrentBridges()
{
    //QFile torrcFile(getTorrcPath());
    QFile torrcFile(torrcPath);

    if (!torrcFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        statusLabel->setText("Cannot read torrc file");
        return;
    }

    QStringList currentBridges;
    QTextStream in(&torrcFile);
    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        if (line.startsWith("Bridge ", Qt::CaseInsensitive)) {
            currentBridges << line;
        }
    }
    torrcFile.close();

    if (!currentBridges.isEmpty()) {
        bridgeTextEdit->setPlainText(currentBridges.join("\n"));
        statusLabel->setText(QString("Loaded %1 existing bridges").arg(currentBridges.size()));
    }
}

QString BridgeManagerDialog::getTorrcPath()
{
    // Try common torrc locations
    QStringList possiblePaths = {
        "/etc/tor/torrc",
        "/etc/torrc",
        "/usr/local/etc/tor/torrc",
        QDir::homePath() + "/.tor/torrc",
        "torrc"  // Current directory
    };

    for (const QString &path : possiblePaths) {
        if (QFile::exists(path)) {
            return path;
        }
    }

    // Default fallback
    return "/etc/tor/torrc";
}


void BridgeManagerDialog::deleteAllBridges()
{
    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this, "Confirm Deletion",
                                "This will remove ALL bridge lines from torrc.\n\n"
                                "Are you sure you want to continue?",
                                QMessageBox::Yes | QMessageBox::No);

    if (reply != QMessageBox::Yes) {
        return;
    }

    if (removeBridgesFromTorrc()) {
        statusLabel->setText("All bridges removed from torrc. Restart Tor to apply.");
        bridgeTextEdit->clear();
        QMessageBox::information(this, "Success",
            "All bridges have been removed from torrc.\n\nYou need to restart Tor for changes to take effect.");
    } else {
        QMessageBox::critical(this, "Error", "Failed to remove bridges from torrc.");
    }
}

bool BridgeManagerDialog::removeBridgesFromTorrc()
{
    QFile torrcFile(torrcPath);
    if (!torrcFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return false;
    }

    // Read all lines, filtering out bridge lines
    QStringList newLines;
    QTextStream in(&torrcFile);

    while (!in.atEnd()) {
        QString line = in.readLine();

        // Skip lines that contain "Bridge" (case-insensitive)
        //if (!line.trimmed().startsWith("Bridge ", Qt::CaseInsensitive)) {
          //  newLines << line;
       // }
        if (line.trimmed().contains("bridge", Qt::CaseInsensitive)) {
                    //newLines << line;
                        continue;
                }
        // Skip ALL empty lines ← SIMPLE!
        if (line.trimmed().isEmpty()) {
                   continue;
               }
        newLines << line;


    }
    torrcFile.close();

    // Write back the filtered content
    if (!torrcFile.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
        return false;
    }

    QTextStream out(&torrcFile);
    for (const QString &line : newLines) {
        out << line << "\n";
    }
    torrcFile.close();

    return true;
}

void BridgeManagerDialog::toggleBridges(bool enabled)
{
    QFile torrcFile(torrcPath);
    if (!torrcFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::critical(this, "Error", "Cannot open torrc file.");
        return;
    }

    // Read all lines
    QStringList lines;
    QTextStream in(&torrcFile);
    while (!in.atEnd()) {
        lines << in.readLine();
    }
    torrcFile.close();

    // Process lines: comment/uncomment bridge lines
    QStringList newLines;
    for (QString line : lines) {
        QString trimmed = line.trimmed();

        if (trimmed.startsWith("Bridge ", Qt::CaseInsensitive) ||
            trimmed.startsWith("# Bridge ", Qt::CaseInsensitive)) {

            if (enabled) {
                // Enable: remove "# " from beginning if present
                if (trimmed.startsWith("# Bridge ", Qt::CaseInsensitive)) {
                    line = line.mid(2); // Remove "# " (2 characters)
                }
            } else {
                // Disable: add "# " to beginning if not already commented
                if (trimmed.startsWith("Bridge ", Qt::CaseInsensitive)) {
                    line = "# " + line;
                }
            }
        }
        newLines << line;
    }

    // Write back to file
    if (!torrcFile.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
        QMessageBox::critical(this, "Error", "Cannot write to torrc file.");
        return;
    }

    QTextStream out(&torrcFile);
    for (const QString &line : newLines) {
        out << line << "\n";
    }
    torrcFile.close();
    // SAVE STATE TO QSETTINGS ← NEW
    settings.setValue("bridges/enabled", enabled);
    statusLabel->setText(enabled ? "Bridges enabled"
                                : "Bridges disabled");

    // Reload to show current state
    loadCurrentBridges();

    //QMessageBox::information(this, "Success",
      //  QString("Bridges have been %1.\n\nRestart Tor for changes to take effect.").arg(enabled ? "enabled" : "disabled"));
}
