#include "securitymanager.h"
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QInputDialog>
#include <QMessageBox>
#include <QCryptographicHash>
#include <QApplication>
#include <QWidget>

SecurityManager::SecurityManager(QWidget* parent)
    : QObject(parent)
    , m_parent(parent)
    , m_requirePasswordAction(nullptr)
    , m_settings(QSettings())
{
}


bool SecurityManager::checkPasswordOnStartup()
{
    if (!isPasswordProtectionEnabled()) {
        return true; // No password required
    }

    return validateMasterPassword();
}

bool SecurityManager::isPasswordProtectionEnabled() const
{
    return m_settings.value("security/passwordProtectionEnabled", false).toBool();
}

void SecurityManager::onTogglePasswordProtection(bool enabled)
{
    if (enabled) {
        // Show info about enabling password protection, with Cancel option
        QMessageBox msgBox(QMessageBox::Information,
                           "Password Protection",
                           "🔒 Enabling Password Protection\n\n"
                           "Selene will now require a master password on startup.\n"
                           "This helps protect your saved websites and data.",
                           QMessageBox::Ok | QMessageBox::Cancel,
                           m_parent);
        int infoResult = msgBox.exec();
        if (infoResult != QMessageBox::Ok) {
            // User cancelled or closed dialog, revert the toggle
            m_requirePasswordAction->setChecked(false);
            return;
        }

        // Enabling password protection - need to set master password if not set
        if (!m_settings.contains("security/masterPasswordHash")) {
            setMasterPassword();
            // If user cancels in setMasterPassword, it will revert the toggle and return
            // So we should check if the password was actually set
            if (!m_settings.contains("security/masterPasswordHash")) {
                m_requirePasswordAction->setChecked(false);
                return;
            }
        }

        m_settings.setValue("security/passwordProtectionEnabled", true);

    } else {
        // Disabling password protection, with Cancel option
        QMessageBox msgBox(QMessageBox::Question,
                           "Disable Password Protection",
                           "Are you sure you want to disable password protection?\n"
                           "This will make your data accessible without a password.",
                           QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel,
                           m_parent);
        msgBox.setDefaultButton(QMessageBox::No);
        int ret = msgBox.exec();
        if (ret == QMessageBox::Yes) {
            m_settings.setValue("security/passwordProtectionEnabled", false);
        } else {
            // User cancelled or chose No, revert the action
            m_requirePasswordAction->setChecked(true);
            return;
        }
    }
}



bool SecurityManager::promptForMasterPassword()
{
    bool ok;
    QString password = QInputDialog::getText(m_parent, "Master Password Required",
                                             "Enter master password to continue:",
                                             QLineEdit::Password, "", &ok);

    if (!ok) {
        return false; // User cancelled
    }

    QString hashedInput = hashPassword(password);
    QString storedHash = m_settings.value("security/masterPasswordHash").toString();

    return hashedInput == storedHash;
}

void SecurityManager::setMasterPassword(bool isEdit)
{
    // Show security notice with Cancel option
    QMessageBox msgBox(QMessageBox::Information,
                       "Set Master Password",
                       "⚠️ IMPORTANT SECURITY NOTICE ⚠️\n\n"
                       "You are about to set a master password.\n\n"
                       "• Choose a strong, memorable password\n"
                       "• Write it down in a safe place\n"
                       "• If you forget it, you'll need to factory reset\n\n"
                       "Click OK to continue...",
                       QMessageBox::Ok | QMessageBox::Cancel,
                       m_parent);
    int noticeResult = msgBox.exec();
    if (noticeResult != QMessageBox::Ok) {
        // User cancelled or closed dialog
        m_requirePasswordAction->setChecked(false);
        return;
    }

    // Prompt for new password
    bool ok = false;
    QString password = QInputDialog::getText(
        m_parent,
        "Set Master Password",
        "Enter new master password:",
        QLineEdit::Password,
        "",
        &ok
        );
    if (!ok) {
        // User cancelled or closed dialog
        m_requirePasswordAction->setChecked(false);
        return;
    }

    // Confirm password
    ok = false;
    QString confirmPassword = QInputDialog::getText(
        m_parent,
        "Confirm Master Password",
        "Confirm your master password:",
        QLineEdit::Password,
        "",
        &ok
        );
    if (!ok) {
        // User cancelled or closed dialog
        m_requirePasswordAction->setChecked(false);
        return;
    }

    if (password != confirmPassword) {
        QMessageBox::warning(m_parent, "Password Mismatch",
                             "Passwords do not match. Please try again.");
        m_requirePasswordAction->setChecked(false);
        return;
    }

    // Save hashed password
    QString hashedPassword = hashPassword(password);
    m_settings.setValue("security/masterPasswordHash", hashedPassword);

    QMessageBox::information(m_parent, "Password Set Successfully",
                             "Master password has been set successfully.\n"
                             "Selene will now require this password on startup.");
}



QString SecurityManager::hashPassword(const QString& password)
{
    QCryptographicHash hash(QCryptographicHash::Sha256);
    hash.addData(password.toUtf8());
    hash.addData("SeleneSalt2024"); // Add salt for security
    return hash.result().toHex();
}


void SecurityManager::setupSecurityMenu(QMenuBar* menuBar)
{
    QMenu* securityMenu = menuBar->addMenu("Security");

    m_requirePasswordAction = new QAction("Require Password on Startup", this);
    m_requirePasswordAction->setCheckable(true);
    m_requirePasswordAction->setChecked(isPasswordProtectionEnabled());
    connect(m_requirePasswordAction, &QAction::toggled,
            this, &SecurityManager::onTogglePasswordProtection);

    QAction* changePasswordAction = new QAction("Change Master Password", this);
    connect(changePasswordAction, &QAction::triggered, [this]() {
        setMasterPassword(true);  // true = editing mode
    });

    securityMenu->addAction(m_requirePasswordAction);
    securityMenu->addAction(changePasswordAction);

}

bool SecurityManager::validateMasterPassword()
{
    for (int attempts = 0; attempts < 5; attempts++) {
        if (promptForMasterPassword()) {
            return true;
        }

        if (attempts < 5) {
            QMessageBox::warning(m_parent, "Incorrect Password",
                                 QString("Incorrect password. %1 attempt(s) remaining.")
                                     .arg(5 - attempts));
        }
    }

    // 3 failed attempts - show options dialog
    QMessageBox msgBox(m_parent);
    msgBox.setIcon(QMessageBox::Critical);
    msgBox.setWindowTitle("Access Denied");
    msgBox.setText("Too many failed password attempts.");
    msgBox.setInformativeText("What would you like to do?");

    QPushButton* exitButton = msgBox.addButton("Exit Application", QMessageBox::RejectRole);
    QPushButton* resetButton = msgBox.addButton("Factory Reset", QMessageBox::DestructiveRole);

    msgBox.setDefaultButton(exitButton);
    msgBox.exec();

    if (msgBox.clickedButton() == static_cast<QAbstractButton*>(resetButton)) {
        emit factoryResetRequested();
        return true;
    }
    return false; // Exit app
}

void SecurityManager::clearSecuritySettings()
{
    m_settings.remove("security/passwordProtectionEnabled");
    m_settings.remove("security/masterPasswordHash");
    m_settings.sync();

    // Update the menu checkbox
    if (m_requirePasswordAction) {
        m_requirePasswordAction->setChecked(false);
    }
}

