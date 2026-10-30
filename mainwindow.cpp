#include "mainwindow.h"
#include "./ui_mainwindow.h"


#include <QWidget>
#include <QDockWidget>
#include<QClipboard>
#include "sitemanager.h"
#include"torprocess.h"
#include<QScrollBar>
#include<QSystemTrayIcon>
#include<QStyle>
#include<QListWidget>
#include<QListWidgetItem>
#include<QStandardPaths>
#include"constants.h"
#include<QCoreApplication>
#include<QFontDatabase>
#include<QFileDialog>
#include<QSettings>
#include<QToolBar>
#include <QStyle>
#include<QRegularExpression>
#include<QMimeData>
#include"messagebubblewidget.h"
#include<QWheelEvent>
#include<QSplitter>
#include<QWidgetAction>
#include"helpmenudialog.h"
#include<QShortcut>
#include"Notification.h"
#include"donationdialog.h"
#include<QCryptographicHash>
#include<QSaveFile>
#include"bridgemanagerdialog.h"
#include<QDesktopServices>
#include<QKeySequence>
#include<QInputDialog>
#include<QSpinBox>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , settings(QSettings())
    , bridgeDialog(new BridgeManagerDialog(this))
    , m_siteManager(new SiteManager(this))

{

    loadPortsFromTorrc();

    Logger::log(Logger::INFO, QString("Default ports: Chat=%1, HTTP=%2, Control=%3, Proxy=%4")
                                  .arg(DefaultChatServicePort)
                                  .arg(DefaultHttpServicePort)
                                  .arg(DefaultControlPort)
                                  .arg(DefaultProxyPort));


    QString docRootPath = getWWWDir();
    m_siteDialog = new SiteManagerDialog(m_siteManager, this);
    m_siteManager->setFileShareRoot(docRootPath);
    httpClientDialog = new HttpClientDialog(m_siteManager, this);
    httpServer = new SimpleHttpFileServer(docRootPath, DefaultHttpServicePort, m_siteManager, this);

    ui->setupUi(this);
    setAcceptDrops(true);

    setWindowTitle("Selene - P2P Chat");
    setWindowIcon(QIcon(":/icons/selene.png"));

    resize(1200, 750);
    isConnected = false;
    connectedPeerAddress = "";
    contactNameLabel = nullptr;
    contactAddressLabel = nullptr;
    contactCommentsLabel = nullptr;

    cleanTmpWWWDirs();

    if (torConfig.setupHiddenService()) {
        QString configPath = getAppDataDir() + "/.selene/torrc";
        if (TorProcess::startTor(configPath)) {
            torConfig.readOnionAddress();
            QString onion = torConfig.getOnionAddress();
            networkManager = new NetworkManager(this);
            networkManager->initialize(onion);
        }
    }

    httpOnionAddress = torConfig.getFileOnionAddress();

    contactManager = networkManager->getContactManager();

    setupMainLayoutWithSplitter();

    connect(networkManager, &NetworkManager::connectionStatusChanged,
            this, [this](bool connected, const QString& peer) {
                handleConnectionStatus(connected, peer);
            });

    connect(networkManager, &NetworkManager::peerDisconnected,
            this, &MainWindow::handlePeerDisconnected);
    connect(networkManager, &NetworkManager::peerConnected,
            this, &MainWindow::handlePeerConnected);
    connect(networkManager, &NetworkManager::networkError,
            this, &MainWindow::handleNetworkError);
    connect(networkManager, &NetworkManager::peerIdentified,
            this, &MainWindow::onPeerIdentified);

    connect(networkManager, &NetworkManager::messageReceived,
            this, &MainWindow::handleMessageReceived);
    connect(this, &MainWindow::unknownContactMessageReceived,
            this, &MainWindow::handleUnknownContact);

    chatManager = new ChatManager(this);

    setupChatManager();
    connect(chatManager, &ChatManager::messageAdded,
            this, &MainWindow::handleMessageAdded);
    QString appDataDir = getAppDataDir();

    QString historyFile = getChatHistoryFilePath();

    if (QFile::exists(historyFile)) {
        bool loaded = chatManager->loadFromFile(historyFile);
    } else {
    }

    connect(contactListWidget, &ContactListWidget::editContactRequested, this, &MainWindow::handleEditContact);
    connect(contactListWidget, &ContactListWidget::blockContactRequested, this, &MainWindow::handleBlockContact);
    connect(contactListWidget, &ContactListWidget::deleteContactRequested, this, &MainWindow::handleDeleteContact);

    connect(networkManager, &NetworkManager::unknownContactMessageReceived,
            this, &MainWindow::handleUnknownContact);

    TorProcess::torInstance = new TorProcess();

    securityManager = new SecurityManager(this);
    connect(securityManager, &SecurityManager::factoryResetRequested,
            this, &MainWindow::performFactoryReset);
    securityManager->checkPasswordOnStartup();

    menubar = this->menuBar();
    setMenuBar(menubar);

    setupActions();

    setupMenuBar();

    toolbar = getToolBar();

    connect(qApp, &QCoreApplication::aboutToQuit, this, &MainWindow::handleAppExit);





    QFile onionFile(getTorChatHiddenDirPath() + "/hostname");
    if (onionFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        myOnion = QString::fromUtf8(onionFile.readAll()).trimmed();
        onionFile.close();
    }

    if (enableClearAllChatsCheckBox->isChecked()) {
        clearAllHistory();
    }
    bool checked = muteSoundsCheckBox->isChecked();
    if(networkManager) {
        networkManager->setPlay(!checked);
        play = !checked;
    }
    QTimer::singleShot(1000, this, [this]() {
    playSound(SoundType::MessageReceived, play);
    });

    bool enabled = enableLoggerCheckBox->isChecked();
    Logger::setLoggingEnabled(enabled);

    QShortcut* increaseFontShortcut = new QShortcut(QKeySequence("Ctrl+="), this);
    connect(increaseFontShortcut, &QShortcut::activated, this, [this]() {
        m_bubbleFontSize = std::min(m_bubbleFontSize + 1, 32);
        updateBubbleFontSizes();
    });

    QShortcut* decreaseFontShortcut = new QShortcut(QKeySequence("Ctrl+-"), this);
    connect(decreaseFontShortcut, &QShortcut::activated, this, [this]() {
        m_bubbleFontSize = std::max(m_bubbleFontSize - 1, 8);
        updateBubbleFontSizes();
    });

    setupCornerWidget();

    checkFirstRun();

}

void MainWindow::setupChatArea() {
    centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);
    QVBoxLayout* layout = new QVBoxLayout(centralWidget);

    QWidget* headerWidget = new QWidget(centralWidget);
    headerWidget->setObjectName("chatHeaderWidget");
    headerWidget->setStyleSheet("#chatHeaderWidget { background-color: #f5f5f5; border-radius: 8px; }");
    headerWidget->setMinimumHeight(70);
    QVBoxLayout* headerLayout = new QVBoxLayout(headerWidget);

    headerLayout->setSpacing(2);        // Space between each row (name, address, comments, group, status)
    headerLayout->setContentsMargins(8, 2, 8, 2);  // left, top, right, bottom

    contactNameLabel = new QLabel("No contact selected", headerWidget);
    QFont nameFont = contactNameLabel->font();
    nameFont.setBold(true);
    nameFont.setPointSize(nameFont.pointSize() + 1);
    contactNameLabel->setFont(nameFont);

    QHBoxLayout* addressLayout = new QHBoxLayout();
    contactAddressLabel = new QLabel("", headerWidget);
    contactAddressLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    QPushButton* copyAddressBtn = new QPushButton(style()->standardIcon(QStyle::SP_DialogSaveButton), "", headerWidget);


    copyAddressBtn->setFixedSize(24, 24);
    copyAddressBtn->setToolTip("Copy address");
    connect(copyAddressBtn, &QPushButton::clicked, this, [this]() {
        QGuiApplication::clipboard()->setText(contactAddressLabel->text());
        statusLabel->setText("Address copied to clipboard");
    });
    addressLayout->addWidget(new QLabel("Address:", headerWidget));
    addressLayout->addWidget(contactAddressLabel);
    addressLayout->addWidget(copyAddressBtn);
    addressLayout->addStretch();

    QHBoxLayout* groupLayout = new QHBoxLayout();
    groupLayout->addWidget(new QLabel("Group:", headerWidget));
    contactGroupLabel = new QLabel("", headerWidget);
    contactGroupLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    groupLayout->addWidget(contactGroupLabel, 1);

    QHBoxLayout* commentsLayout = new QHBoxLayout();
    commentsLayout->addWidget(new QLabel("Comments:", headerWidget));
    contactCommentsLabel = new QLabel("", headerWidget);
    contactCommentsLabel->setWordWrap(true);
    contactCommentsLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    commentsLayout->addWidget(contactCommentsLabel, 1);

    statusLabel = new QLabel("Mode: Tor", headerWidget);
    statusLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);

    QHBoxLayout* nameLayout = new QHBoxLayout();
    nameLayout->addWidget(contactNameLabel);
    nameLayout->addStretch();

    headerLayout->addLayout(nameLayout);        // 1. Name first
    headerLayout->addLayout(addressLayout);     // 2. Address second
    headerLayout->addLayout(groupLayout);       // 3. Group third
    headerLayout->addLayout(commentsLayout);    // 4. Comments fourth
    headerLayout->addWidget(statusLabel);       // 5. Status last

    QWidget* connectionWidget = new QWidget(centralWidget);
    QHBoxLayout* connectionLayout = new QHBoxLayout(connectionWidget);
    peerAddressInput = new QLineEdit(connectionWidget);
    peerAddressInput->setPlaceholderText("Enter peer's onion address");

    connectButton = new QPushButton("Connect", connectionWidget);
    openChatButton = new QPushButton("Open Chat", connectionWidget);
    openChatButton->setToolTip("Open chat tab without connecting");

    connectionLayout->addWidget(peerAddressInput);
    connectionLayout->addWidget(connectButton);
    connectionLayout->addWidget(openChatButton);

    chatTabWidget = new QTabWidget(centralWidget);
    chatTabWidget->setTabsClosable(true);

    connect(openChatButton, &QPushButton::clicked, this, [this]() {
        QString address = peerAddressInput->text().trimmed();
        if (address.isEmpty())
            return;
        openChatTabForPeer(address);
    });

    connect(chatTabWidget, &QTabWidget::tabCloseRequested, this, [this](int index) {
        QString onion = chatTabWidget->tabToolTip(index);
        chatDisplays.remove(onion);
        chatTabWidget->removeTab(index);
    });

    chatDisplay = new QTextBrowser();

    inputWidget = new QWidget(centralWidget);
    QHBoxLayout* inputLayout = new QHBoxLayout(inputWidget);
    messageInput = new QTextEdit(inputWidget);
    messageInput->setFixedHeight(50);

    messageInput->setPlaceholderText("Type your message...\n'Send' button or Ctrl+Enter to send");
    sendButton = new QPushButton("Send", inputWidget);
    fileButton = new QPushButton("Send File", inputWidget);
    encryptCheckBox = new QCheckBox(inputWidget);
    encryptCheckBox->setToolTip("Encrypt messages to this peer");

    encryptCheckBox->setChecked(false); // Default: not checked
    connect(encryptCheckBox, &QCheckBox::toggled, this, &MainWindow::onEncryptToggled);


    setupEmojiPickerDialog();

    inputLayout->addWidget(messageInput);
    inputLayout->addWidget(emojiButton);
    inputLayout->addWidget(sendButton);
    inputLayout->addWidget(fileButton);
    inputLayout->addWidget(encryptCheckBox);

    layout->addWidget(headerWidget);
    layout->addWidget(connectionWidget);
    layout->addWidget(chatTabWidget, 1); // Give chat tab widget more stretch
    layout->addWidget(inputWidget);

    connect(sendButton, &QPushButton::clicked, this, &MainWindow::sendMessage);


    auto ctrlShortcutReturn = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_Return), this);
    connect(ctrlShortcutReturn, &QShortcut::activated, this, [this]() {
        if (sendButton) sendButton->click();
    });

    connect(fileButton, &QPushButton::clicked, this, &MainWindow::sendFile);
    connect(connectButton, &QPushButton::clicked, this, &MainWindow::connectToPeer);
    connect(chatTabWidget, &QTabWidget::currentChanged, this, &MainWindow::handleTabChanged);

    QString onion = torConfig.getOnionAddress();
    if (!onion.isEmpty()) {
        updateStatus("Connected as: " + onion);
    }


}



MainWindow::~MainWindow() {
    if (networkManager) {
        networkManager->cleanup();
    }

    if(TorProcess::torInstance){
        delete TorProcess::torInstance;
        TorProcess::torInstance = nullptr;
    }
    if (emojiPicker) {
        delete emojiPicker;
        emojiPicker = nullptr;
    }
    if (pickerDialog) {
        delete pickerDialog;
        pickerDialog = nullptr;
    }
    if (httpServer) {
        httpServer->stop();
        delete httpServer;
        httpServer = nullptr;
    }
    delete ui;

}

void MainWindow::updateStatus(const QString& status) {
    statusLabel->setText(status);
}


void MainWindow::connectToPeer() {


    QString address = peerAddressInput->text().trimmed();

    if (networkManager->isPeerConnected(address)) {
        networkManager->setActiveConnection(address);  // Add this line

        networkManager->disconnectFromPeer(address);
        return;
    }

    if (address.isEmpty()) {
        return;
    }

    if (networkManager->isPeerConnected(address)) {
        QMessageBox::information(this, "Already Connected",
                                 "You are already connected to this peer. Switching to existing conversation.");
        switchToTab(address);
        return;
    }



    QApplication::processEvents();

    statusBar()->showMessage("Connecting to peer. Please wait up to 30 seconds...", 6 * 1000);
    Notification("Connecting", "Connecting to peer. Please wait up to 30 seconds...");



    if (networkManager->connectToPeer(address)) {
        if (!chatDisplays.contains(address)) {
            createChatTab(address);

        } else {
            switchToTab(address);

        }

        if (!chatManager->getAllSessionPeers().contains(address)) {
            QString peerName = networkManager->getFriendlyName(address);
            chatManager->createSession(address, peerName);
        }

        chatManager->activateSession(address);
    } else {

    }
}



void MainWindow::handleConnectionStatus(bool connected, const QString& peerAddress) {
    if (peerAddressInput->text().trimmed() == peerAddress) {
        isConnected = connected;
        connectButton->setText(connected ? "Disconnect" : "Connect");
        peerAddressInput->setEnabled(!connected);
    }
    updateStatus(connected ? "Connected" : "Disconnected");
}






void MainWindow::handlePeerConnected(const QString& peerAddress) {


    if (peerAddressInput->text().trimmed() == peerAddress) {
        isConnected = true;
        connectButton->setText("Disconnect");
        peerAddressInput->setEnabled(false);
    }
    updateStatus("Connected");

    if (contactListWidget) {
        contactListWidget->setContactConnected(peerAddress);
    }

    if (!chatDisplays.contains(peerAddress)) {
        createChatTab(peerAddress);
    }
    switchToTab(peerAddress);

    if (!chatManager->getAllSessionPeers().contains(peerAddress)) {
        QString peerName = networkManager->getFriendlyName(peerAddress);
        chatManager->createSession(peerAddress, peerName);
    }
    chatManager->activateSession(peerAddress);
    connectedPeerAddress = peerAddress;


    onContactSelected(peerAddress);
}

void MainWindow::handlePeerDisconnected(const QString& peerAddress) {
    if (peerAddressInput->text().trimmed() == peerAddress) {
        isConnected = false;
        connectButton->setText("Connect");
        peerAddressInput->setEnabled(true);
    }
    updateStatus("Peer disconnected");

    if (!peerAddress.isEmpty() && contactListWidget) {
        contactListWidget->setContactDisconnected(peerAddress);
    }

}

void MainWindow::handleNetworkError(const QString& error) {

    updateStatus("Error: " + error);
    isConnected = false;
    connectButton->setText("Connect");
    peerAddressInput->setEnabled(true);
}

void MainWindow::onPeerIdentified(const QString& peerOnion) {
    statusLabel->setText("Connected");

    if (!chatManager->getAllSessionPeers().contains(peerOnion)) {
        QString peerName = networkManager->getFriendlyName(peerOnion);
        chatManager->createSession(peerOnion, peerName);
    }

    chatManager->activateSession(peerOnion);

}


void MainWindow::onAddContactClicked() {

    addContact(QString());
}




void MainWindow::onContactSelected(const QString& onionAddress) {
    peerAddressInput->setText(onionAddress);

    bool isPeerConnected = networkManager->isPeerConnected(onionAddress);
    connectButton->setText(isPeerConnected ? "Disconnect" : "Connect");
    peerAddressInput->setEnabled(!isPeerConnected);

    Contact contact = contactManager->getContact(onionAddress);

    if (!contact.friendlyName.isEmpty()) {
        contactNameLabel->setText(contact.friendlyName);
    } else {
        contactNameLabel->setText("Unknown Contact");
    }
    contactAddressLabel->setText(onionAddress);

    if (!contact.group.isEmpty()) {
        contactGroupLabel->setText(contact.group);
    } else {
        contactGroupLabel->setText("Ungrouped");
    }

    if (!contact.comments.isEmpty()) {
        contactCommentsLabel->setText(contact.comments);
    } else {
        contactCommentsLabel->setText("No comments");
    }

    /*
    if (!contact.friendlyName.isEmpty()) {
        statusLabel->setText("Selected contact: " + contact.friendlyName);
    } else {
        statusLabel->setText("Selected contact: " + onionAddress);
    }
    */
    statusLabel->setText(isPeerConnected ? "Connected" : "Disconnected");



    for (int i = 0; i < chatTabWidget->count(); i++) {
        if (chatTabWidget->tabToolTip(i) == onionAddress) {
            chatTabWidget->setCurrentIndex(i);
            break;
        }
    }
    contactListWidget->setSelectedContact(onionAddress);
    encryptCheckBox->blockSignals(true);
    encryptCheckBox->setChecked(contact.encryptionEnabled);
    encryptCheckBox->blockSignals(false);
}



void MainWindow::updateContactsList() {
    if (contactListWidget) {
        contactListWidget->refreshContacts(networkManager->getContacts());

        const QStringList connecterPeers = networkManager->getConnectedPeers();
        for (const auto &peer : connecterPeers) {
            contactListWidget->setContactConnected(peer);
        }

    }
}

void MainWindow::setupDockWidgets() {
    contactsDock = new QDockWidget(tr("Contacts"), this);
    contactsDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    contactsDock->setFeatures(QDockWidget::DockWidgetMovable);
    contactsDock->setMinimumWidth(40);

    dockContent = new QWidget(contactsDock);
    dockLayout = new QVBoxLayout(dockContent);
    searchContactsEdit = new QLineEdit(dockContent);
    searchContactsEdit->setPlaceholderText(tr("🔍 Search contacts"));

    searchContactsEdit->setToolTip(
        "Contact Filter Syntax:\n\n"
        "• [text] - Filter by display name\n"
        "• #b - Show only blocked contacts\n"
        "• #c [text] - Search in comments\n"
        "• #g [text] - Search in group\n"  // <-- ADD THIS
        "• #o [text] - Search in onion address"
    );

    dockLayout->addWidget(searchContactsEdit);


    contactListWidget = new ContactListWidget(dockContent);
    dockLayout->addWidget(contactListWidget);

    QPushButton* addContactBtn = new QPushButton(tr("+ Add Contact"), dockContent);
    dockLayout->addWidget(addContactBtn);

    contactsDock->setWidget(dockContent);
    addDockWidget(Qt::LeftDockWidgetArea, contactsDock);

    connect(searchContactsEdit, &QLineEdit::textChanged,
            contactListWidget, &ContactListWidget::filterContacts);
    connect(addContactBtn, &QPushButton::clicked,
            this, &MainWindow::onAddContactClicked);
    connect(contactListWidget, &ContactListWidget::contactSelected,
            this, &MainWindow::onContactSelected);
    connect(contactListWidget, &ContactListWidget::connectRequested,
            this, &MainWindow::connectToContact);



    contactListWidget->refreshContacts(networkManager->getContacts());
}


void MainWindow::setupChatManager()
{
    connect(chatManager, &ChatManager::messageAdded,
            this, [this](const QString& peerAddress, const ChatMessage& message) {

                if (chatDisplays.contains(peerAddress)) {
                    QScrollArea* display = chatDisplays[peerAddress];  // Changed from QTextBrowser*


                    chatManager->renderMessageToScrollArea(peerAddress, display, message, m_bubbleFontSize);  // Changed method call for appending single msg

                    int currentIndex = chatTabWidget->currentIndex();
                    for (int i = 0; i < chatTabWidget->count(); i++) {
                        QWidget* widget = chatTabWidget->widget(i);
                        QScrollArea* tabScrollArea = qobject_cast<QScrollArea*>(  // Changed from QTextBrowser*
                            widget->layout()->itemAt(0)->widget());

                        if (tabScrollArea == display && i != currentIndex) {
                            QString friendlyName = networkManager->getFriendlyName(peerAddress);
                            chatTabWidget->setTabText(i, "* " + friendlyName);
                            break;
                        }
                    }
                }
            });
}

void MainWindow::connectToContact(const QString& onionAddress) {
    onContactSelected(onionAddress);
    statusLabel->setText(tr("Connecting to %1...").arg(onionAddress));

    peerAddressInput->setText(onionAddress);

    connectToPeer();

    if (isConnected) {
        QString peerName = networkManager->getFriendlyName(onionAddress);
        chatManager->createSession(onionAddress, peerName);
        chatManager->activateSession(onionAddress);
    }
}

void MainWindow::handleIncomingMessage(const QString& senderAddress, const QString& message) {
    QString actualSenderAddress = senderAddress;

    if (actualSenderAddress.isEmpty()) {
        actualSenderAddress = networkManager->getConnectedPeerAddress();

        if (actualSenderAddress.isEmpty()) {
            return;
        }
    }

    if (!chatManager->getAllSessionPeers().contains(actualSenderAddress)) {
        QString senderName = networkManager->getFriendlyName(actualSenderAddress);

        if (senderName.isEmpty()) {
            senderName = actualSenderAddress;
            emit unknownContactMessageReceived(actualSenderAddress);
        }

        chatManager->createSession(actualSenderAddress, senderName);
    }

    if (chatManager->getActiveSessionPeer() != actualSenderAddress) {
        chatManager->activateSession(actualSenderAddress);
    }

    chatManager->addMessage(actualSenderAddress, message, false);

}


void MainWindow::handleUnknownContact(const QString& senderAddress) {


    createChatTab(senderAddress);
    switchToTab(senderAddress);
    Notification("Unknown Peer", "Received a message from: " + senderAddress);

    addContact(senderAddress);
}

void MainWindow::handleMessageAdded(const QString& peerAddress, const ChatMessage& message) {
    if (chatManager->getActiveSessionPeer() == peerAddress) {
    }

}


void MainWindow::createChatTab(const QString& onionAddress) {
    if (chatDisplays.contains(onionAddress)) {
        switchToTab(onionAddress);
        return;
    }

    QString friendlyName = networkManager->getFriendlyName(onionAddress);
    if (friendlyName.isEmpty() || friendlyName == onionAddress) {
        friendlyName = "Unknown (" + onionAddress.left(8) + "...)";
    }

    QWidget* tabWidget = new QWidget(chatTabWidget);
    chatTabWidgets[onionAddress] = tabWidget;


    QVBoxLayout* tabLayout = new QVBoxLayout(tabWidget);

    scrollArea = new QScrollArea(tabWidget);

    scrollArea->setWidgetResizable(true);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    scrollArea->setStyleSheet("QScrollArea { border: none; background-color: #f5f5f5; }");

    tabLayout->addWidget(scrollArea);

    chatDisplays[onionAddress] = scrollArea;

    int index = chatTabWidget->addTab(tabWidget, friendlyName);
    chatTabWidget->setTabToolTip(index, onionAddress);

    chatTabWidget->setCurrentIndex(index);

    updateContactInfo(onionAddress);

    if (chatManager->getAllSessionPeers().contains(onionAddress)) {
        chatManager->renderMessagesToScrollArea(onionAddress, scrollArea, m_bubbleFontSize);

    }
    QScrollBar* vScrollBar = scrollArea->verticalScrollBar();
    if (vScrollBar) {
        vScrollBar->setValue(vScrollBar->maximum());
    }
}



void MainWindow::switchToTab(const QString& onionAddress) {
    if (!chatDisplays.contains(onionAddress)) {
        return;
    }
    QScrollArea* textBrowser = chatDisplays[onionAddress];  // CHANGED type but kept name

    int index = -1;

    for (int i = 0; i < chatTabWidget->count(); i++) {
        if (chatTabWidget->widget(i)->layout()->itemAt(0)->widget() == textBrowser) {
            index = i;
            break;
        }
    }

    if (index >= 0) {
        chatTabWidget->setCurrentIndex(index);
        updateContactInfo(onionAddress);
        QScrollBar *vScrollBar = textBrowser->verticalScrollBar();
        if (vScrollBar) {
            vScrollBar->setValue(vScrollBar->maximum());
        }
    }

}

void MainWindow::handleEditContact(const QString &onion) {
    onContactSelected(onion);
    contactListWidget->onContactSelected();

    QDialog dialog(this);
    QFormLayout form(&dialog);

    Contact currentContact = contactManager->getContact(onion);

    QLineEdit* onionInput = new QLineEdit(currentContact.onionAddress, &dialog);
    onionInput->setReadOnly(true);
    QLineEdit* nameInput = new QLineEdit(currentContact.friendlyName, &dialog);
    QTextEdit* publicKeyInput = new QTextEdit(currentContact.publicKey, &dialog);
    bool encryptionEnabled = currentContact.encryptionEnabled;
    QLineEdit* commentsInput = new QLineEdit(currentContact.comments, &dialog);
    QLineEdit* groupInput = new QLineEdit(currentContact.group, &dialog);

    form.addRow("Onion Address:", onionInput);
    form.addRow("Name:", nameInput);
    form.addRow("Public Key:", publicKeyInput);
    form.addRow("Comments:", commentsInput);
    form.addRow("Group:", groupInput);

    QDialogButtonBox buttonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, &dialog);
    form.addRow(&buttonBox);

    connect(&buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(&buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    if (dialog.exec() == QDialog::Accepted) {
        Contact updatedContact;
        updatedContact.onionAddress = onionInput->text();
        updatedContact.friendlyName = nameInput->text();
        QString newKey = publicKeyInput->toPlainText();

        updatedContact.publicKey = newKey.isEmpty() ? currentContact.publicKey : newKey;
        updatedContact.comments = commentsInput->text();
        updatedContact.group = groupInput->text();
        updatedContact.encryptionEnabled = encryptionEnabled;

        contactManager->updateContact(updatedContact);
        Notification("Contact Updated", "Updated contact: " + updatedContact.friendlyName);


        updateContactsList();

        if (chatManager->getChatSessions().contains(onion)) {
            chatManager->getChatSessions()[onion].peerName = updatedContact.friendlyName;
        }        
    }
}



void MainWindow::addContact(const QString &sendersOnion, const QJsonObject &contact)
{
    lastAddedContactFriendlyName.clear();
    if (m_addContactDialogsOpen.contains(sendersOnion)) {
        return;
    }
    m_addContactDialogsOpen.insert(sendersOnion);


    QDialog* dialog = new QDialog(this);
    QFormLayout* form = new QFormLayout(dialog);

    QLineEdit* onionInput = new QLineEdit(dialog);
    QLineEdit* nameInput = new QLineEdit(dialog);
    QTextEdit* publicKeyInput = new QTextEdit(dialog);
    QLineEdit* commentsInput = new QLineEdit(dialog);
    QLineEdit* groupInput = new QLineEdit(dialog);


    onionInput->setMinimumWidth(70 * fontMetrics().horizontalAdvance('X'));
    onionInput->setPlaceholderText("**Required");
    nameInput->setMinimumWidth(70 * fontMetrics().horizontalAdvance('X'));
    nameInput->setPlaceholderText("**Required");

    publicKeyInput->setMinimumWidth(70 * fontMetrics().horizontalAdvance('X'));

    commentsInput->setMinimumWidth(70 * fontMetrics().horizontalAdvance('X'));
    groupInput->setMinimumWidth(70 * fontMetrics().horizontalAdvance('X'));

    form->addRow("Onion Address:", onionInput);
    form->addRow("Friendly Name:", nameInput);
    form->addRow("Public Key:", publicKeyInput);
    form->addRow("Comments:", commentsInput);
    form->addRow("Group:", groupInput);

    if (!sendersOnion.isEmpty()) {
        onionInput->setText(sendersOnion);
    }

    if (!contact.isEmpty()) {
        if (contact.contains("onion"))
            onionInput->setText(contact.value("onion").toString());
        if (contact.contains("name"))
            nameInput->setText(contact.value("name").toString());
        if (contact.contains("public_key"))
            publicKeyInput->setPlainText(contact.value("public_key").toString());
        if (contact.contains("comments"))
            commentsInput->setText(contact.value("comments").toString());
        if (contact.contains("group"))
            groupInput->setText(contact.value("group").toString());
    }



    QPushButton* importContactBtn = new QPushButton("Import Contact", dialog);
    importContactBtn->setToolTip("Import Contact from a File");
    form->addRow(importContactBtn);

    connect(importContactBtn, &QPushButton::clicked, this, [=]() {
        QString filePath = QFileDialog::getOpenFileName(
            dialog,
            tr("Import Contact"),
            getContactsDirPath(),
            tr("Contact Files (*.contact)")
            );
        if (filePath.isEmpty())
            return;

        QFile file(filePath);
        if (!file.open(QIODevice::ReadOnly)) {
            QMessageBox::warning(dialog, tr("Import Failed"), tr("Could not open contact file."));
            return;
        }
        QByteArray data = file.readAll();
        file.close();

        QJsonParseError parseError;
        QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);
        if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
            QMessageBox::warning(dialog, tr("Import Failed"), tr("Invalid contact file format."));
            return;
        }
        QJsonObject obj = doc.object();

        if (!obj.contains("onion") || !obj.contains("name")) {
            QMessageBox::warning(dialog, tr("Import Failed"), tr("Contact file missing required fields."));
            return;
        }

        onionInput->setText(obj.value("onion").toString());
        nameInput->setText(obj.value("name").toString());
        publicKeyInput->setPlainText(obj.value("public_key").toString());
        if (obj.contains("comments"))
            commentsInput->setText(obj.value("comments").toString());
        else
            commentsInput->clear();

        if (obj.contains("group"))
            groupInput->setText(obj.value("group").toString());
        else
            groupInput->clear();

    });


    QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, dialog);
    form->addRow(buttonBox);

    connect(buttonBox, &QDialogButtonBox::rejected, dialog, &QDialog::reject);
    connect(buttonBox, &QDialogButtonBox::accepted, this, [=]() {
        if (onionInput->text().isEmpty() || nameInput->text().isEmpty()) {
            QMessageBox::warning(dialog, tr("Missing Fields"), tr("Onion and name are mandatory."));
            return; // Do NOT call dialog->accept(), so dialog stays open
        }
        if (!onionInput->text().contains(".onion", Qt::CaseInsensitive)) {
            QMessageBox::warning(dialog, tr("Invalid Onion Address"), tr("The onion address must contain '.onion'."));
            return;
        }

        if (nameInput->text().contains(".onion", Qt::CaseInsensitive)) {
            QMessageBox::warning(dialog, tr("Invalid Name"), tr("The name must not contain '.onion'."));
            return;
        }
        dialog->accept();
    });
    connect(dialog, &QDialog::accepted, this, [=]() {
        if(onionInput->text().isEmpty() || nameInput->text().isEmpty()) {
            QMessageBox::warning(dialog, tr("Missing Fields"), tr("Onion and name are mandatory."));
            return;
        }



        Contact newContact;
        newContact.onionAddress = onionInput->text();

        newContact.friendlyName = nameInput->text();
        newContact.publicKey = publicKeyInput->toPlainText();

        newContact.comments = commentsInput->text();
        newContact.group = groupInput->text();

        contactManager->addContact(newContact);
        Notification("Contact Added", "Added contact: " + newContact.friendlyName);


        updateContactsList();

        if (chatManager->getChatSessions().contains(sendersOnion)) {
            chatManager->getChatSessions()[sendersOnion].peerName = newContact.friendlyName;
        }

        QWidget* tabWidget = chatTabWidgets.value(sendersOnion, nullptr);
        if (tabWidget) {
            int idx = chatTabWidget->indexOf(tabWidget);
            if (idx != -1)
                chatTabWidget->setTabText(idx, lastAddedContactFriendlyName);
        }
    });


    dialog->setAttribute(Qt::WA_DeleteOnClose); // Ensure dialog is deleted when closed
    dialog->show(); // Non-modal
    dialog->raise();
    dialog->activateWindow();

    connect(dialog, &QDialog::finished, this, [=](int) {
        m_addContactDialogsOpen.remove(sendersOnion);
        dialog->deleteLater();
    });
}

void MainWindow::handleBlockContact(const QString &onion) {
    onContactSelected(onion);
    contactListWidget->onContactSelected();

    bool currentlyBlocked = contactManager->isBlocked(onion);
    QString action = currentlyBlocked ? "unblock" : "block";

    if (QMessageBox::question(this,
                              QString("%1 Contact").arg(action.toUpper()),
                              QString("Are you sure you want to %1 this contact?").arg(action),
                              QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes) {

        contactManager->setBlocked(onion, !currentlyBlocked);
        Notification("Contact Blocked", "Blocked contact: " + onion);

        if (!currentlyBlocked) {
            networkManager->disconnectFromPeer(onion);
        }

    }
    updateContactsList();

}




void MainWindow::handleDeleteContact(const QString &onion)
{
    onContactSelected(onion);
    contactListWidget->onContactSelected();  // Add this after

    if (QMessageBox::question(this, "Delete Contact",
                              "Are you sure you want to delete this contact?") == QMessageBox::Yes) {
        networkManager->disconnectFromPeer(onion);
        contactManager->deleteContact(onion);
        Notification("Contact Deleted", "Deleted contact: " + onion);

        updateContactsList();

    }
}


void MainWindow::handleTabChanged(int index) {
    if (index < 0) return;

    QString onionAddress;
    QWidget* tabWidget = chatTabWidget->widget(index);
    QScrollArea* scrollArea = qobject_cast<QScrollArea*>(  // CHANGED from QTextBrowser*
        tabWidget->layout()->itemAt(0)->widget());

    for (auto it = chatDisplays.begin(); it != chatDisplays.end(); ++it) {
        if (it.value() == scrollArea) {  // CHANGED from textBrowser
            onionAddress = it.key();
            break;
        }
    }

    if (!onionAddress.isEmpty()) {
        networkManager->setActiveConnection(onionAddress);

        chatManager->activateSession(onionAddress);

        updateContactInfo(onionAddress);

        if (chatManager->getAllSessionPeers().contains(onionAddress)) {
            chatManager->renderMessagesToScrollArea(onionAddress, scrollArea, m_bubbleFontSize);  // CHANGED method and parameter
            chatManager->markAsRead(onionAddress);
        }

        QString friendlyName = networkManager->getFriendlyName(onionAddress);
        chatTabWidget->setTabText(index, friendlyName);

        if (networkManager->isPeerConnected(onionAddress)) {
        } else {
        }


    }
    peerAddressInput->setText(onionAddress);
    onContactSelected(onionAddress);
}


void MainWindow::sendMessage() {
    QString message = messageInput->toPlainText().trimmed();
    if (message.isEmpty()) return;

    QString currentOnion = networkManager->getConnectedPeerAddress();
    if (currentOnion.isEmpty()) {
        QMessageBox::warning(this, "Cannot Send", "No active connection.");
        return;
    }

    QString unencryptedMessage = message;
    if (encryptCheckBox->isChecked()) {
        QString encrypted = encryptMessageForPeer(currentOnion, message);
        if (encrypted.isEmpty()) {
            QMessageBox::warning(this, "Encryption Error", "Failed to encrypt the message.");
            return;
        }
        message = encrypted;
    }

        networkManager->sendMessage(message);

    if (chatManager->getAllSessionPeers().contains(currentOnion)) {
        chatManager->addMessage(currentOnion, unencryptedMessage, true);
    } else {
        QString peerName = networkManager->getFriendlyName(currentOnion);
        chatManager->createSession(currentOnion, peerName);
        chatManager->addMessage(currentOnion, unencryptedMessage, true);
    }



    QScrollBar* vScrollBar = scrollArea->verticalScrollBar();
    if (vScrollBar) {
        vScrollBar->setValue(vScrollBar->maximum());
    }

    messageInput->clear();
}

void MainWindow::updateContactInfo(const QString& onionAddress) {
    if (onionAddress.isEmpty()) {
        contactNameLabel->setText("No contact selected");
        contactAddressLabel->setText("");
        contactCommentsLabel->setText("");
        contactGroupLabel->setText("");
        return;
    }

    QString friendlyName = networkManager->getFriendlyName(onionAddress);
    QString comments = "";
    QString group = "";

    ContactManager* contactManager = networkManager->getContactManager();
    if (contactManager) {
        Contact contact = contactManager->getContact(onionAddress);
        if (!contact.comments.isEmpty()) {
            comments = contact.comments;
        }
        if (!contact.group.isEmpty()) {
            group = contact.group;
        }
    }

    contactNameLabel->setText(friendlyName);
    contactAddressLabel->setText(onionAddress);
    contactCommentsLabel->setText(comments);
    contactGroupLabel->setText(group.isEmpty() ? "Ungrouped" : group);
    if (networkManager->isPeerConnected(onionAddress)) {
    } else {
    }

    setWindowTitle("Selene - P2PChat - " + friendlyName);
}

void MainWindow::handleMessageReceived(const QString& senderOnion, const QString& message) {
    QString displayMessage = message;

    if (!chatDisplays.contains(senderOnion)) {
        createChatTab(senderOnion);
    }

    if (!chatManager->getAllSessionPeers().contains(senderOnion)) {
        QString senderName = networkManager->getFriendlyName(senderOnion);
        if (senderName.isEmpty()) {
            senderName = senderOnion;
        }
        chatManager->createSession(senderOnion, senderName);
    }

    if (contactManager->getContact(senderOnion).encryptionEnabled) {
        QString decrypted = decryptMessageFromPeer(senderOnion, message);
        if (!decrypted.isEmpty()) {
            displayMessage = decrypted;
        } else {
            displayMessage = "[Failed to decrypt message]";
        }
    }
    chatManager->addMessage(senderOnion, displayMessage, false);



    int tabIndex = -1;
    QScrollArea* textBrowser = chatDisplays[senderOnion];
    for (int i = 0; i < chatTabWidget->count(); i++) {
        QWidget* widget = chatTabWidget->widget(i);
        QScrollArea* tabBrowser = qobject_cast<QScrollArea*>(
            widget->layout()->itemAt(0)->widget());
        if (tabBrowser == textBrowser) {
            tabIndex = i;
            break;
        }
    }

    if (tabIndex >= 0 && tabIndex != chatTabWidget->currentIndex()) {
        chatTabWidget->setTabText(tabIndex, "* " + networkManager->getFriendlyName(senderOnion));
    }

    /*
    QScrollBar* vScrollBar = scrollArea->verticalScrollBar();
    if (vScrollBar) {
        vScrollBar->setValue(vScrollBar->maximum());
    }
    */

    if (chatDisplays.contains(senderOnion)) {
        QScrollArea* scrollArea = chatDisplays[senderOnion];  // Get the right scroll area
        QScrollBar* vScrollBar = scrollArea->verticalScrollBar();
        if (vScrollBar) {
            vScrollBar->setValue(vScrollBar->maximum());
        }
    }
}

QToolBar* MainWindow::getToolBar() {
    toolbar = addToolBar("Main Toolbar");

    QLabel *torLabel = new QLabel(this);
    torLabel->setText("Tor");
    toolbar->addWidget(torLabel);
    toolbar->addAction(newCircuitAction);
    toolbar->addAction(editTorrcPortsAction);
    toolbar->addAction(newHttpOnionAction);
    toolbar->addAction(newChatOnionAction);
    toolbar->addAction(openBridgeDialogAction);

    toolbar->addSeparator();

    QLabel *httpServerLabel = new QLabel(this);
    httpServerLabel->setText("Http");
    toolbar->addWidget(httpServerLabel);
    toolbar->addAction(getHttpOnionAction);
    toolbar->addAction(getWWWDirAction);
    toolbar->addAction(startServerAction);
    toolbar->addAction(stopServerAction);
    toolbar->addAction(restartServerAction);
    toolbar->addAction(openHttpClientAction);
    toolbar->addAction(openSiteManagerAction);
    toolbar->addSeparator();
    QLabel *chatLabel = new QLabel(this);
    chatLabel->setText("Chat");
    toolbar->addWidget(chatLabel);
    toolbar->addAction(copyOnionAction);
    toolbar->addAction(getContactsDirPathAction);
    toolbar->addAction(clearCurrentHistoryAction);
    toolbar->addAction(clearAllHistoryAction);
    statusCombo = new QComboBox(this);
    statusCombo->setObjectName("statusCombo");
    statusCombo->addItem("Available", true);  // true for Available
    statusCombo->addItem("Away", false);      // false for Away
    statusCombo->setToolTip("Set your status");
    connect(statusCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int) {
                bool isAvailable = this->statusCombo->currentData().toBool();
                this->networkManager->availableForChat = isAvailable;
            });
    toolbar->addWidget(statusCombo);
    addFontSizeCombo();
    toolbar->addAction(exportMyInfoAction);

    toolbar->addSeparator();
    QLabel *encryptionLabel = new QLabel(this);
    encryptionLabel->setText("RSA");
    toolbar->addWidget(encryptionLabel);
    toolbar->addAction(copyPubKeyAction);
    toolbar->addAction(createKeypairAction);
    rsaBitsCombo = new QComboBox(this);
    rsaBitsCombo->setObjectName("rsaBitsCombo");
    rsaBitsCombo->setToolTip("Select number of bits to be used with the new RSA key");
    rsaBitsCombo->addItem("2048", 2048);
    rsaBitsCombo->addItem("4096", 4096);
    rsaBitsCombo->addItem("8192", 8192);
    toolbar->addWidget(rsaBitsCombo);

    toolbar->addSeparator();
    QLabel *loggerLabel = new QLabel(this);
    loggerLabel->setText("Logger");
    toolbar->addWidget(loggerLabel);
    toolbar->addAction(toggleLoggingAction);
    toolbar->addAction(logViewerAction);
    toolbar->addSeparator();
    toolbar->addAction(muteAction);

    return toolbar;
}


void MainWindow::handleAppExit()
{
    if (settings.value("autoClearHistoryOnExit", false).toBool()) {
        clearAllHistory();
    }
    chatManager->saveToFile(getChatHistoryFilePath());

}

void MainWindow::openChatTabForPeer(const QString& onionAddress) {
    if (!chatDisplays.contains(onionAddress)) {
        createChatTab(onionAddress);
    }
    switchToTab(onionAddress);


}

QStringList MainWindow::getAllEmojiResources() {
    return QStringList()
           << "😀" << "😁" << "😂" << "🤣" << "😃" << "😄" << "😅" << "😆" << "😉" << "😊"
           << "😋" << "😎" << "😍" << "😘" << "🥰" << "😗" << "😙" << "😚" << "🙂" << "🤗"
           << "🤩" << "🤔" << "🤨" << "😐" << "😑" << "😶" << "🙄" << "😏" << "😣" << "😥"
           << "😮" << "🤐" << "😯" << "😪" << "😫" << "🥱" << "😴" << "😌" << "😛" << "😜"
           << "😝" << "🤤" << "😒" << "😓" << "😔" << "😕" << "🙃" << "🤑" << "😲" << "☹️"
           << "🙁" << "😖" << "😞" << "😟" << "😤" << "😢" << "😭" << "😦" << "😧" << "😨"
           << "😩" << "🤯" << "😬" << "😰" << "😱" << "🥵" << "🥶" << "😳" << "🤪" << "😵"
           << "🥴" << "😠" << "😡" << "🤬" << "😷" << "🤒" << "🤕" << "🤢" << "🤮" << "🤧"
           << "😇" << "🥳" << "🥺" << "🤠" << "🤡" << "🤥" << "🤫" << "🤭" << "🧐" << "🤓"
           << "😈" << "👿" << "👹" << "👺" << "💀" << "👻" << "👽" << "🤖" << "💩" << "😺"
           << "😸" << "😹" << "😻" << "😼" << "😽" << "🙀" << "😿" << "😾" << "🙈" << "🙉"
           << "🙊" << "💋" << "💌" << "💘" << "💝" << "💖" << "💗" << "💓" << "💞" << "💕"
           << "💟" << "❣️" << "💔" << "❤️" << "🧡" << "💛" << "💚" << "💙" << "💜" << "🤎"
           << "🖤" << "🤍" << "💯" << "💢" << "💥" << "💫" << "💦" << "💨" << "🕳️" << "💣"
           << "💬" << "👋" << "🤚" << "🖐️" << "✋" << "🖖" << "👌" << "🤌" << "🤏" << "✌️"
           << "🤞" << "🤟" << "🤘" << "🤙" << "👈" << "👉" << "👆" << "🖕" << "👇" << "☝️"
           << "👍" << "👎" << "✊" << "👊" << "🤛" << "🤜" << "👏" << "🙌" << "👐" << "🤲"
           << "🙏" << "✍️" << "💅" << "🤳" << "💪" << "🦾" << "🦵" << "🦶" << "👂" << "🦻"
           << "👃" << "🧠" << "🦷" << "🦴" << "👀" << "👁️" << "👅" << "👄" << "💋" << "🩸"
           << "🎉";
}

void MainWindow::setupEmojiPickerDialog()
{
    emojiPaths = getAllEmojiResources();

    emojiPicker = new EmojiPickerWidget(emojiPaths, this);

    int fontId = QFontDatabase::addApplicationFont(":/emojis/NotoColorEmoji.ttf");
    if (fontId == -1) {
        qWarning() << "Failed to load NotoColorEmoji.ttf!";
    }
    QStringList loadedFamilies = QFontDatabase::applicationFontFamilies(fontId);
    if (!loadedFamilies.isEmpty()) {
    }

    QFont font = messageInput->font();
    font.setFamily(font.family() + ", Noto Color Emoji");
    font.setPointSize(12); // Adjust as needed
    messageInput->setFont(font);

    connect(emojiPicker, &EmojiPickerWidget::emojiSelected, this, [this](const QString& emoji) {
        QTextCursor cursor = messageInput->textCursor();
        cursor.insertText(emoji);
        messageInput->setTextCursor(cursor);

        if (emojiPicker->parentWidget() && emojiPicker->parentWidget()->inherits("QDialog")) {
            emojiPicker->parentWidget()->close();
        }
    });

    emojiButton = new QPushButton(inputWidget);
    emojiButton->setText("😊"); // Or set an icon if you prefer
    emojiButton->setFixedWidth(32);

    pickerDialog = new QDialog(nullptr);
    pickerDialog->setWindowFlags(Qt::Dialog);
    pickerDialog->setAttribute(Qt::WA_DeleteOnClose, false); // We want to reuse the dialog
    pickerDialog->setAttribute(Qt::WA_TranslucentBackground, true);

    pickerDialog->setStyleSheet(R"(
        QDialog {
            background: #23272e;
            border-radius: 8px;
            border: 1px solid #444;
        }
        QPushButton {
            background: transparent;
            border-radius: 4px;
        }
        QPushButton:hover {
            background: #2c313a;
        }
    )");

    QScrollArea* scrollArea = new QScrollArea(pickerDialog);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    emojiPicker->setParent(scrollArea);
    scrollArea->setWidget(emojiPicker);

    QVBoxLayout* dlgLayout = new QVBoxLayout(pickerDialog);
    dlgLayout->setContentsMargins(8, 8, 8, 8);
    dlgLayout->addWidget(scrollArea);

    pickerDialog->setFixedSize(350, 300);

    connect(emojiButton, &QPushButton::clicked, this, [this]() {
        QPoint globalPos = emojiButton->mapToGlobal(QPoint(0, emojiButton->height()));
        pickerDialog->move(globalPos);
        pickerDialog->raise();
        pickerDialog->activateWindow();
        pickerDialog->show();
    });
}



void MainWindow::cleanTmpWWWDirs()
{
    QDir wwwDir(getWWWDir());
    if (!wwwDir.exists())
        return;

    QFileInfoList entries = wwwDir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot);

    for (const QFileInfo& entry : entries) {
        if (entry.isDir()) {
            QDir subDir(entry.absoluteFilePath());
            if (!subDir.removeRecursively()) {
                qWarning() << "Failed to remove directory:" << subDir.absolutePath();
            }
        }
    }
}



void MainWindow::sendFile()
{
    QString startDir = getFileSaveDirPath();
    QStringList filePaths = QFileDialog::getOpenFileNames(this, tr("Select File(s) to Share"), startDir);

    if (filePaths.isEmpty())
        return;

    QString peerOnion = networkManager->getConnectedPeerAddress();
    if (peerOnion.isEmpty()) {
        QMessageBox::warning(this, tr("No Connection"),
                           tr("No active peer connection. Please connect to a peer first."));
        return;
    }

    QString wwwDir = getWWWDir();
    QString uuidDirName = QUuid::createUuid().toString(QUuid::WithoutBraces);
    QString destDirPath = wwwDir + QDir::separator() + uuidDirName;
    QDir().mkpath(destDirPath);

    for (const QString& filePath : filePaths) {
        QFileInfo fileInfo(filePath);
        QString destFilePath;

        if (encryptCheckBox->isChecked()) {
            QString encryptedBundlePath = encryptFileForPeer(filePath, peerOnion);
            if (encryptedBundlePath.isEmpty()) {
                QMessageBox::warning(this, tr("Encryption Error"),
                                   tr("Failed to encrypt file: %1").arg(fileInfo.fileName()));
                QDir(destDirPath).removeRecursively();
                return;
            }

            destFilePath = destDirPath + QDir::separator() + fileInfo.fileName() + ".encbundle";
            if (!QFile::copy(encryptedBundlePath, destFilePath)) {
                QMessageBox::warning(this, tr("File Copy Error"),
                                   tr("Failed to copy encrypted file %1").arg(fileInfo.fileName()));
                QDir(destDirPath).removeRecursively();
                QFile::remove(encryptedBundlePath);
                return;
            }

            QFile::remove(encryptedBundlePath);

        } else {
            destFilePath = destDirPath + QDir::separator() + fileInfo.fileName();
            if (!QFile::copy(filePath, destFilePath)) {
                QMessageBox::warning(this, tr("File Copy Error"),
                                   tr("Failed to copy %1 to %2").arg(filePath, destFilePath));
                QDir(destDirPath).removeRecursively();
                return;
            }
        }
    }

    QString onionBase = torConfig.getFileOnionAddress();
    QString url = onionBase + "/" + uuidDirName + "/";

    QString messageText = url;
    if (encryptCheckBox->isChecked()) {
    }

    messageInput->setPlainText(messageText);
    sendMessage();  // This will handle encryption of the URL message if needed

    if (httpServer->isRunning()) {
        httpServer->restart();
    } else {
        httpServer->start();
    }

    QString confirmation = tr("Files have been shared at:\n%1").arg(url);
    if (encryptCheckBox->isChecked()) {
        confirmation += tr("\n\nFiles are encrypted for the current peer.");
    }

    QMessageBox::information(this, tr("Files Shared"), confirmation);
}



void MainWindow::playSound(SoundType type, bool playIt)
{
    if (!playIt) return;

    if (!m_soundEffects.contains(type)) {
        QSoundEffect* effect = new QSoundEffect(this);
        effect->setSource(QUrl("qrc" + soundResourcePath(type)));
        effect->setVolume(0.8);
        m_soundEffects[type] = effect;
    }
    QSoundEffect* effect = m_soundEffects[type];
    if (effect->isPlaying()) {
        effect->stop();
    }
    effect->play();
}


void MainWindow::clearCurrentTabHistory() {
    int index = chatTabWidget->currentIndex();
    if (index < 0) return;

    QString onion = chatTabWidget->tabToolTip(index);
    if (onion.isEmpty()) return;

    if (chatDisplays.contains(onion)) {
        QScrollArea* scrollArea = chatDisplays[onion];
        QWidget* container = scrollArea->widget();
        if (container) {
            QLayout* layout = container->layout();
            if (layout) {
                while (QLayoutItem* item = layout->takeAt(0)) {
                    if (QWidget* w = item->widget()) {
                        w->deleteLater();
                    }
                    delete item;
                }
            }
            container->update();
        }
    }

    chatManager->clearHistoryForPeer(onion);
}

void MainWindow::clearAllHistory() {
    for (auto display : chatDisplays) {
        QScrollArea* scrollArea = display;
        QWidget* container = scrollArea->widget();
        if (container) {
            QLayout* layout = container->layout();
            if (layout) {
                while (QLayoutItem* item = layout->takeAt(0)) {
                    if (QWidget* w = item->widget()) {
                        w->deleteLater();
                    }
                    delete item;
                }
            }
            container->update();
        }
    }

    chatManager->clearAllHistory();

    QFile::remove(getChatHistoryFilePath());
}

void MainWindow::setupMenuBar()
{

    QMenu* fileMenu = menuBar()->addMenu("&File");
    QAction *openFolderAction = fileMenu->addAction("&Open Data Directory");
    connect(openFolderAction, &QAction::triggered, this, &MainWindow::openFolder);
    fileMenu->addSeparator();

    QAction *createSymlinkAction = fileMenu->addAction("Create Shortcut to Selene Data");
    connect(createSymlinkAction, &QAction::triggered, this, &MainWindow::createSymlink);
    fileMenu->addSeparator();

    fileMenu->addAction("&Exit", this, &QWidget::close, QKeySequence::Quit);
    QMenu* torMenu = menubar->addMenu(tr("Tor"));
    newCircuitAction->setText(tr("New Tor Circuit"));
    torMenu->addAction(newCircuitAction);

    QMenu* httpMenu = menubar->addMenu(tr("HTTP Server"));



    getHttpOnionAction->setText(tr("Copy Server Onion"));
    httpMenu->addAction(getHttpOnionAction);

    getWWWDirAction->setText(tr("Copy WWW Dir"));
    httpMenu->addAction(getWWWDirAction);

    httpMenu->addSeparator();

    startServerAction->setText(tr("Start Server"));
    httpMenu->addAction(startServerAction);

    stopServerAction->setText(tr("Stop Server"));
    httpMenu->addAction(stopServerAction);

    restartServerAction->setText(tr("Restart Server"));
    httpMenu->addAction(restartServerAction);

    newHttpOnionAction->setText(tr("New HTTP Onion"));
    httpMenu->addAction(newHttpOnionAction);

    openHttpClientAction->setText("Open Http Client");
    httpMenu->addAction(openHttpClientAction);

    openSiteManagerAction->setText("Open Websites Manager");
    httpMenu->addAction(openSiteManagerAction);

    QMenu* chatMenu = menubar->addMenu(tr("Chat"));
    clearCurrentHistoryAction->setText(tr("Clear Current Chat"));
    chatMenu->addAction(clearCurrentHistoryAction);

    clearAllHistoryAction->setText(tr("Clear All Chats"));
    chatMenu->addAction(clearAllHistoryAction);

    chatMenu->addSeparator();

    newChatOnionAction->setText(tr("New Chat Onion"));
    chatMenu->addAction(newChatOnionAction);

    copyPubKeyAction->setText(tr("Copy My Public Key"));
    chatMenu->addAction(copyPubKeyAction);

    createKeypairAction->setText(tr("Create New Key Pair"));
    chatMenu->addAction(createKeypairAction);

    exportMyInfoAction->setText(tr("Export My Info"));
    chatMenu->addAction(exportMyInfoAction);

    copyOnionAction->setText(tr("Copy Chat Onion URL"));
    chatMenu->addAction(copyOnionAction);

    QMenu* settingsMenu = menubar->addMenu(tr("Settings"));
    factoryResetAction->setText(tr("Factory Reset"));
    settingsMenu->addAction(factoryResetAction);

    settingsMenu->addSeparator();



    QWidgetAction* enableClearAllChatsWidgetAction = new QWidgetAction(this);
    enableClearAllChatsCheckBox = new QCheckBox(tr("Enable 'Clear All Chats'"));

    bool clearAllChatsEnabled = settings.value("clearAllChatsEnabled", true).toBool();
    enableClearAllChatsCheckBox->setChecked(clearAllChatsEnabled);
    enableClearAllChatsWidgetAction->setDefaultWidget(enableClearAllChatsCheckBox);
    settingsMenu->addAction(enableClearAllChatsWidgetAction);

    connect(enableClearAllChatsCheckBox, &QCheckBox::toggled, this, [this](bool checked) {

        settings.setValue("clearAllChatsEnabled", checked);
        clearAllHistoryAction->setEnabled(checked);


    });


    QWidgetAction* muteSoundsWidgetAction = new QWidgetAction(this);
    muteSoundsCheckBox = new QCheckBox(tr("Mute Sounds"));


    bool muteEnabled = settings.value("muteSoundsEnabled", false).toBool();
    muteSoundsCheckBox->setChecked(muteEnabled);
    muteAction->setChecked(muteEnabled);

    muteSoundsWidgetAction->setDefaultWidget(muteSoundsCheckBox);
    settingsMenu->addAction(muteSoundsWidgetAction);

    connect(muteSoundsCheckBox, &QCheckBox::toggled, this, [this](bool checked) {

        settings.setValue("muteSoundsEnabled", checked);
        muteAction->setChecked(checked);
    });

    QWidgetAction* enableLoggerWidgetAction = new QWidgetAction(this);
    enableLoggerCheckBox = new QCheckBox(tr("Enable Logger"));

    bool loggerEnabled = settings.value("loggerEnabled", true).toBool();
    enableLoggerCheckBox->setChecked(loggerEnabled);
    toggleLoggingAction->setChecked(loggerEnabled);

    enableLoggerWidgetAction->setDefaultWidget(enableLoggerCheckBox);
    settingsMenu->addAction(enableLoggerWidgetAction);

    connect(enableLoggerCheckBox, &QCheckBox::toggled, this, [this](bool checked) {

        settings.setValue("loggerEnabled", checked);
        toggleLoggingAction->setChecked(checked);
    });




    if (securityManager) {
        securityManager->setupSecurityMenu(menubar);
    }

    if (logViewerAction && !settingsMenu->actions().contains(logViewerAction)) {
        logViewerAction->setText(tr("View Log"));
        settingsMenu->addAction(logViewerAction);
    }

    if (editTorrcPortsAction && !settingsMenu->actions().contains(editTorrcPortsAction)) {
        editTorrcPortsAction->setText(tr("Edit Torrc Ports"));
        settingsMenu->addAction(editTorrcPortsAction);
    }
    settingsMenu->addSeparator();
    openBridgeDialogAction->setText("Open Bridge manager");
    settingsMenu->addAction(openBridgeDialogAction);


    settingsMenu->addSeparator();
    QAction* scaleAction = settingsMenu->addAction("Set &Scale...");
    scaleAction->setShortcut(QKeySequence("Ctrl+Shift+S"));
    connect(scaleAction, &QAction::triggered, this, [this]() {
        QSettings settings;
        bool ok = false;
        double current = settings.value("ui/scaleFactor", 1.0).toDouble();

        double factor = QInputDialog::getDouble(
            this, tr("UI Scale"),
            tr("Scale factor (e.g. 0.9, 1.0, 1.1, 1.25):"),
            current,          // initial value
            0.5,              // min
            3.0,              // max
            2,                // decimals shown
            &ok,
            Qt::WindowFlags(),
            0.05);             // ← step

        if (!ok) return;

        settings.setValue("ui/scaleFactor", factor);
        settings.sync();

        QMessageBox msg(this);
        msg.setWindowTitle(tr("Restart Required"));
        msg.setIcon(QMessageBox::Information);
        msg.setText(tr("Please restart the application to apply the new scale."));
        msg.setInformativeText(tr(
            "If the new scale makes the app unusable, delete the settings file:\n\n%1\n\n"
            "Note: this will reset all user-defined settings.")
            .arg(QSettings().fileName()));


        QPushButton *copyBtn   = msg.addButton(tr("Copy Command"), QMessageBox::ActionRole);
        QPushButton *cancelBtn = msg.addButton(tr("Cancel"), QMessageBox::RejectRole);
        QPushButton *okBtn     = msg.addButton(tr("OK"), QMessageBox::AcceptRole);

        for (;;) {
            msg.exec();
            if (msg.clickedButton() != copyBtn)
                break;

        #ifdef Q_OS_WIN
            const QString cmd = QString("Remove-Item \"%1\"").arg(QSettings().fileName());
        #else
            const QString cmd = QString("rm \"%1\"").arg(QSettings().fileName());
        #endif
            QGuiApplication::clipboard()->setText(cmd);

            msg.setInformativeText(tr(
                "Command copied to clipboard:\n\n%1\n\n"
                "If the new scale makes the app unusable, delete the settings file above.")
                .arg(cmd));
        }

        if (msg.clickedButton() == okBtn)
            qApp->quit();

    });

    settingsMenu->addSeparator();
    QAction* fontAction = settingsMenu->addAction("Set &Font Size...");
    fontAction->setShortcut(QKeySequence("Ctrl+Shift+F"));
    connect(fontAction, &QAction::triggered, this, [this]() {
        QSettings settings;

        QMessageBox box(this);
        box.setWindowTitle(tr("Font Size"));
        box.setIcon(QMessageBox::NoIcon);
        box.setText(tr("Point size (e.g. 9, 10, 12, 14):"));

        QSpinBox *spin = new QSpinBox(&box);
        spin->setRange(6, 32);
        spin->setSingleStep(1);

        double current = settings.value("ui/fontSize", 0.0).toDouble();
        if (current <= 0.0)
            current = qApp->font().pointSizeF();
        spin->setValue(qRound(current));

        if (auto *grid = qobject_cast<QGridLayout*>(box.layout())) grid->addWidget(spin, 1, 1);
        QPushButton *okBtn     = box.addButton(QMessageBox::Ok);
        QPushButton *cancelBtn = box.addButton(QMessageBox::Cancel);
        QPushButton *defaultBtn = box.addButton(tr("Default"), QMessageBox::ResetRole);
        box.setDefaultButton(okBtn);

        box.exec();

        qreal size;
        if (box.clickedButton() == defaultBtn) {
            size = DEFAULTFONTSIZE;
        } else if (box.clickedButton() == okBtn) {
            size = spin->value();
        } else {
            return;   // Cancel
        }

        FONTSIZE = size;
        settings.setValue("ui/fontSize", size);
        settings.sync();

        QFont f = qApp->font();
        f.setPointSizeF(size);
        qApp->setFont(f);
        qApp->setStyleSheet(qApp->styleSheet());
    });

    settingsMenu->addSeparator();
    QAction *resetAction = settingsMenu->addAction(tr("Reset Settings..."));
    connect(resetAction, &QAction::triggered, this, [this]() {
        QSettings s;
        const QString path = s.fileName();

        const auto reply = QMessageBox::warning(
            this,
            tr("Reset Settings"),
            tr("This will delete your settings file:\n\n%1\n\n"
               "Restart the application for changes to take effect.\n\n"
               "Continue?").arg(path),
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::No);

        if (reply != QMessageBox::Yes)
            return;

        s.sync();                          // flush pending writes

        const bool ok = !QFile::exists(path) || QFile::remove(path);

        if (ok) {
            QMessageBox::information(
                this,
                tr("Settings Reset"),
                tr("Settings deleted.\n\nRestart the app for changes to take effect."));
        } else {
            QMessageBox::critical(
                this,
                tr("Settings Reset Failed"),
                tr("Could not delete:\n\n%1\n\nCheck file permissions.").arg(path));
        }
    });

    QMenu* helpMenu = menubar->addMenu(tr("Help"));

    QAction* aboutAction = new QAction(tr("About"), this);
    helpMenu->addAction(aboutAction);

    connect(aboutAction, &QAction::triggered, this, [this]() {
        HelpMenuDialog dialog(HelpType::About, this);
        dialog.exec();
    });

    QAction* featuresAction = new QAction(tr("Features"), this);
    helpMenu->addAction(featuresAction);

    connect(featuresAction, &QAction::triggered, this, [this]() {
        HelpMenuDialog dialog(HelpType::Features, this);
        dialog.exec();
    });

    QAction* changelogAction = new QAction(tr("Changelog"), this);
    helpMenu->addAction(changelogAction);

    connect(changelogAction, &QAction::triggered, this, [this]() {
        HelpMenuDialog dialog(HelpType::WhatsNew, this);
        dialog.exec();
    });

    QAction* instructionsAction = new QAction(tr("Instructions"), this);
    helpMenu->addAction(instructionsAction);

    connect(instructionsAction, &QAction::triggered, this, [this]() {
        HelpMenuDialog dialog(HelpType::Instructions, this);
        dialog.exec();
    });

    QAction* supportusAction = new QAction(tr("Support Us"), this);
    helpMenu->addAction(supportusAction);

    connect(supportusAction, &QAction::triggered, this, [this]() {
        DonationDialog dialog(this);
        dialog.exec();
    });

}

void MainWindow::setupActions()
{
    QStyle* style = this->style();

    newCircuitAction = new QAction(QIcon(":/icons/refresh-cw.svg"), tr(""), this);

    newCircuitAction->setObjectName("newcircuit");
    newCircuitAction->setToolTip(tr("Request a new Tor circuit"));
    connect(newCircuitAction, &QAction::triggered, this, []() {
        TorProcess::torInstance->newTorCircuit();
    });

    getHttpOnionAction = new QAction(QIcon(":/icons/copy.svg"), tr(""), this);

    getHttpOnionAction->setObjectName("gethttponion");
    getHttpOnionAction->setToolTip(tr("Copy HTTP server onion address"));
    connect(getHttpOnionAction, &QAction::triggered, this, [this]() {
        QString onionAddress = torConfig.getFileOnionAddress();
        httpOnionAddress = onionAddress;
        QClipboard* clipboard = QGuiApplication::clipboard();
        clipboard->setText(onionAddress);
        QMessageBox::information(this, tr("Onion Address Copied"),
                                 tr("The HTTP file-sharing onion address has been copied to your clipboard:\n%1").arg(onionAddress));
    });

    getWWWDirAction = new QAction(QIcon(":/icons/folder.svg"), tr(""), this);

    getWWWDirAction->setObjectName("getwwwdir");
    getWWWDirAction->setToolTip(tr("Copy HTTP server WWW directory path"));
    connect(getWWWDirAction, &QAction::triggered, this, [this]() {
        QString wwwDirPath = getWWWDir();
        QClipboard* clipboard = QGuiApplication::clipboard();
        clipboard->setText(wwwDirPath);
        QMessageBox::information(this, tr("WWW Directory path copied"),
                                 tr("The HTTP server file-sharing path has been copied to your clipboard:\n%1").arg(wwwDirPath));
    });

    getContactsDirPathAction = new QAction(QIcon(":/icons/folder.svg"), tr(""), this);

    getContactsDirPathAction->setObjectName("getcontactsdir");
    getContactsDirPathAction->setToolTip(tr("Copy Contacts directory path to clipboard"));
    connect(getContactsDirPathAction, &QAction::triggered, this, [this]() {
        QString contactsPath = getContactsDirPath();
        QClipboard* clipboard = QGuiApplication::clipboard();
        clipboard->setText(contactsPath);
        QMessageBox::information(this, tr("Contacts Directory path copied"),
                                 tr("The Contacts directory path has been copied to your clipboard:\n%1").arg(contactsPath));
    });

    startServerAction = new QAction(QIcon(":/icons/play.svg"), tr(""), this);

    startServerAction->setObjectName("startserver");
    startServerAction->setToolTip(tr("Start HTTP server"));
    connect(startServerAction, &QAction::triggered, this, [this]() {
        httpServer->start();
        QMessageBox::information(this, tr("HTTP Server started"), tr("The HTTP server has started\n"));
        Notification(tr("HTTP Server started"), tr("The HTTP server has started\n"));
    });

    stopServerAction = new QAction(QIcon(":/icons/stop-circle.svg"), tr(""), this);

    stopServerAction->setObjectName("stopserver");
    stopServerAction->setToolTip(tr("Stop HTTP server"));
    connect(stopServerAction, &QAction::triggered, this, [this]() {
        httpServer->stop();
        QMessageBox::information(this, tr("HTTP Server stopped"), tr("The HTTP server has stopped\n"));
        Notification(tr("HTTP Server stopped"), tr("The HTTP server has stopped\n"));
    });

    restartServerAction = new QAction(QIcon(":/icons/refresh-cw.svg"), tr(""), this);

    restartServerAction->setObjectName("restartserver");
    restartServerAction->setToolTip(tr("Restart HTTP server"));
    connect(restartServerAction, &QAction::triggered, this, [this]() {
        httpServer->restart();
        QMessageBox::information(this, tr("HTTP Server restarted"), tr("The HTTP server has restarted\n"));
        Notification(tr("HTTP Server restarted"), tr("The HTTP server has restarted\n"));

    });


    newHttpOnionAction = new QAction(QIcon(":/icons/folder-plus.svg"), tr(""), this);

    newHttpOnionAction->setObjectName("newhttponion");
    newHttpOnionAction->setToolTip(tr("Request a new Tor HTTP server onion address"));
    connect(newHttpOnionAction, &QAction::triggered, this, [this]() {
        QMessageBox::StandardButton reply = QMessageBox::question(
            this,
            tr("Confirm New HTTP Onion"),
            tr("This will stop the HTTP server and reset its onion address.\n"
               "A new onion address will be generated after you restart the application.\n\n"
               "Do you want to continue?"),
            QMessageBox::Yes | QMessageBox::No
            );
        if (reply != QMessageBox::Yes) return;
        if (httpServer->isRunning()) httpServer->stop();
        QString hiddenDir = getTorHttpHiddenDirPath();
        QDir dir(hiddenDir);
        if (dir.exists()) dir.removeRecursively();
        QMessageBox::information(
            this,
            tr("New HTTP Onion Requested"),
            tr("The HTTP server's onion address has been reset.\n"
               "A new onion address will be available after you restart the application.")
            );
    });


    openHttpClientAction = new QAction(QIcon(":/icons/chrome.svg"), tr(""), this);

    openHttpClientAction->setObjectName("openhttpclient");
    openHttpClientAction->setToolTip(tr("Open Http Client"));
    connect(openHttpClientAction, &QAction::triggered, this, [this]() {
        httpClientDialog->show();
    });

    clearCurrentHistoryAction = new QAction(QIcon(":/icons/trash.svg"), tr(""), this);

    clearCurrentHistoryAction->setObjectName("clearcurrentchat");
    clearCurrentHistoryAction->setToolTip(tr("Clear current chat history"));


    connect(clearCurrentHistoryAction, &QAction::triggered, this, [this]() {
        clearCurrentTabHistory();
        QMessageBox::information(this, tr("Chat Cleared"), tr("Current chat history has been cleared."));
    });

    clearAllHistoryAction = new QAction(QIcon(":/icons/trash-2.svg"), tr(""), this);

    clearAllHistoryAction->setObjectName("clearallchats");
    clearAllHistoryAction->setToolTip(tr("Clear all chat histories"));
    connect(clearAllHistoryAction, &QAction::triggered, this, [this]() {
        clearAllHistory();
        QMessageBox::information(this, tr("All Chats Cleared"), tr("All chat histories have been cleared."));
    });


    newChatOnionAction = new QAction(QIcon(":/icons/file-plus.svg"), tr(""), this);

    newChatOnionAction->setObjectName("newchatonion");
    newChatOnionAction->setToolTip(tr("Request a new Tor chat service onion address"));
    connect(newChatOnionAction, &QAction::triggered, this, [this]() {
        QMessageBox::StandardButton reply = QMessageBox::critical(
            this,
            tr("Confirm New Chat Onion"),
            tr("WARNING: This will reset your chat service's onion address.\n"
               "A new onion address will be generated after you restart the application.\n\n"
               "Your peers will NOT be able to connect to you until they update to your new address.\n\n"
               "Do you want to continue?"),
            QMessageBox::Yes | QMessageBox::No
            );
        if (reply != QMessageBox::Yes) return;
        QString hiddenDir = getTorChatHiddenDirPath();
        QDir dir(hiddenDir);
        if (dir.exists()) dir.removeRecursively();
        QMessageBox::information(
            this,
            tr("New Chat Onion Requested"),
            tr("The chat service's onion address has been reset.\n"
               "A new onion address will be available after you restart the application.\n\n"
               "Remember to share your new address with your peers so they can reconnect.")
            );
    });

    factoryResetAction = new QAction(QIcon(":/icons/rotate-ccw.svg"), tr(""), this);

    factoryResetAction->setObjectName("factoryreset");
    factoryResetAction->setToolTip(tr("Delete all app data and restore to factory settings"));
    connect(factoryResetAction, &QAction::triggered, this, &MainWindow::performFactoryReset);




    toggleMenuBarAction = new QAction(QIcon(":/icons/menu.svg"), tr(""), this);

    toggleMenuBarAction->setObjectName("togglemenubar");
    toggleMenuBarAction->setToolTip(tr("Show or hide the menu bar"));
    toggleMenuBarAction->setCheckable(true);
    toggleMenuBarAction->setChecked(menubar->isVisible());


    connect(toggleMenuBarAction, &QAction::toggled, this, [this](bool checked) {
        menubar->setVisible(checked);
    });


    copyPubKeyAction = new QAction(QIcon(":/icons/clipboard.svg"), tr(""), this);

    copyPubKeyAction->setObjectName("copypublickey");
    copyPubKeyAction->setToolTip("Copy your public key to clipboard");

    connect(copyPubKeyAction, &QAction::triggered, this, [this]() {
        QString pubKey = crypt.getPublicKey();

        if (pubKey.isEmpty()) {
            QMessageBox::warning(this, "No Public Key", "You have not generated a key pair yet.");
            return;
        }

        QClipboard* clipboard = QGuiApplication::clipboard();
        clipboard->setText(pubKey);

        statusLabel->setText("Public key copied to clipboard");
    });
    createKeypairAction = new QAction(QIcon(":/icons/key.svg"), tr(""), this);
    createKeypairAction->setObjectName("createnewkeypair");
    createKeypairAction->setToolTip("Generate a new public/private key pair (will overwrite existing keys)");

    connect(createKeypairAction, &QAction::triggered, this, [this]() {
        this->onCreateNewKeypair();
    });
    exportMyInfoAction = new QAction(QIcon(":/icons/file-text.svg"), tr(""), this);
    exportMyInfoAction->setObjectName("exportmyinfo");
    exportMyInfoAction->setToolTip("Export your contact information to a .contact file");

    connect(exportMyInfoAction, &QAction::triggered, this, [this]() {
        this->exportMyInfo();
    });
    copyOnionAction = new QAction(QIcon(":/icons/copy.svg"), tr(""), this);
    copyOnionAction->setObjectName("copyonionaddress");
    copyOnionAction->setToolTip("Copy your Chat service onion address to clipboard");

    connect(copyOnionAction, &QAction::triggered, this, [this]() {
        QString myOnion;
        QFile onionFile(getTorChatHiddenDirPath() + "/hostname");
        if (onionFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            myOnion = QString::fromUtf8(onionFile.readAll()).trimmed();
            onionFile.close();
        }
        if (!myOnion.isEmpty()) {
            QClipboard *clipboard = QApplication::clipboard();
            clipboard->setText(myOnion);
            QMessageBox::information(this, tr("Copied"), tr("Your onion address has been copied to the clipboard:\n\n%1").arg(myOnion)
);
        } else {
            QMessageBox::warning(this, tr("Copy Failed"), tr("Could not retrieve your onion address."));
        }
    });
    toggleLoggingAction = new QAction(QIcon(":/icons/stop-circle.svg"), tr(""), this);
    toggleLoggingAction->setObjectName("togglelogging");
    toggleLoggingAction->setToolTip("Enable or disable application logging");
    toggleLoggingAction->setCheckable(true);
    toggleLoggingAction->setChecked(true); // Assume logging is enabled by default

    connect(toggleLoggingAction, &QAction::toggled, this, [this](bool checked) {
        Logger::setLoggingEnabled(checked);
        if (checked) {
            toggleLoggingAction->setIcon(QIcon(":/icons/stop-circle.svg"));
            toggleLoggingAction->setToolTip("Stop Logging");
            Notification("Logging Enabled", "Application logging is now enabled.");


            QMessageBox::information(nullptr, QObject::tr("Logging Enabled"), QObject::tr("Application logging is now enabled."));
        } else {
            toggleLoggingAction->setIcon(QIcon(":/icons/play-circle.svg"));
            toggleLoggingAction->setToolTip("Start Logging");

            Notification("Logging Disabled", "Application logging is now disabled.");


        }
    });
    editTorrcPortsAction = new QAction(QIcon(":/icons/edit.svg"), tr(""), this);
    editTorrcPortsAction->setObjectName("editTorrcPorts");
    editTorrcPortsAction->setToolTip(
        "Edit the inner ports in your torrc file.\n"
        "In most cases, you will never need to change these.\n"
        "An exception being if you are running another Tor-based app that uses the same ports.\n"
        "You will know this is necessary if Selene fails to create onion addresses for the Chat and HTTP services."
        );

    connect(editTorrcPortsAction, &QAction::triggered, this, [this]() {
        this->editTorrcPortsDialog();
    });
    logViewerAction = new QAction(QIcon(":/icons/eye.svg"), tr(""), this);
    logViewerAction->setShortcut(QKeySequence("Ctrl+L"));
    connect(logViewerAction, &QAction::triggered, this, [this]() {
        QString logPath = getAppDataDir() + "/chat.log";
        LogViewerDialog dlg(logPath, this);

        bool checked = toggleLoggingAction->isChecked();
        toggleLoggingAction->setChecked(false);
        dlg.exec();
        toggleLoggingAction->setChecked(checked);

    });
    muteAction = new QAction(QIcon(":/icons/volume-2.svg"), tr("Mute"), this);
    muteAction->setCheckable(true);
    muteAction->setChecked(false); // Start unchecked (not muted)

    muteAction->setShortcut(QKeySequence("Ctrl+M"));

    connect(muteAction, &QAction::toggled, this, [this](bool checked) {
        if (networkManager) networkManager->setPlay(!checked);

        if(checked) {
            muteAction->setToolTip("Unmute");
            muteAction->setIcon(QIcon(":/icons/volume-x.svg"));
        }else {
            muteAction->setToolTip("Mute");
            muteAction->setIcon(QIcon(":/icons/volume-2.svg"));
            if (networkManager) networkManager->setPlay(true);

        }
    });

    openBridgeDialogAction = new QAction(QIcon(":/icons/shield.svg"), tr(""), this);
    openBridgeDialogAction->setObjectName("openbridgedialog");
    openBridgeDialogAction->setToolTip("Open Bridge Dialog");
    connect(openBridgeDialogAction, &QAction::triggered, [this] {

        bridgeDialog->show();
    });

    openSiteManagerAction = new QAction(QIcon(":/icons/globe.svg"),"Open Websites Manager",  this);
    openSiteManagerAction->setToolTip("Open Websites Manager");
    connect(openSiteManagerAction, &QAction::triggered, this, [this]{

        m_siteDialog->show();
        m_siteDialog->raise();

    });
}



void MainWindow::onCreateNewKeypair()
{
    if (!rsaBitsCombo) {
        QMessageBox::critical(this, "Error", "RSA bits combo box not found.");
        return;
    }
    int rsaBits = rsaBitsCombo->currentData().toInt();
    if (rsaBits == 0) {
        QMessageBox::critical(this, "Error", "Please select a valid RSA key size.");
        return;
    }

    if (crypt.keysExist()) {
        auto reply = QMessageBox::question(
            this,
            "Overwrite Key Pair?",
            "A key pair already exists.\n"
            "Generating a new one will replace your current keys and may make old messages unreadable.\n"
            "Do you want to continue?",
            QMessageBox::Ok | QMessageBox::Cancel
            );
        if (reply != QMessageBox::Ok)
            return;
        if (crypt.generateKeyPair(rsaBits, true)) {
            QMessageBox::information(this, "Key Pair", "New key pair generated successfully.");
        } else {
            QMessageBox::critical(this, "Key Pair", "Failed to generate key pair.");
        }
        return;
    }

    if (crypt.generateKeyPair(rsaBits)) {
        QMessageBox::information(this, "Key Pair", "New key pair generated successfully.");
    } else {
        QMessageBox::critical(this, "Key Pair", "Failed to generate key pair.");
    }
}

QString MainWindow::encryptMessageForPeer(const QString& peerOnion, const QString& plainText)
{
    QString peerPublicKey = contactManager->getContact(peerOnion).publicKey;


    QByteArray encrypted;

    bool ok = crypt.encrypt(plainText.toUtf8(), peerPublicKey, encrypted);

    if (!ok) {
        return QString();
    }


    return QString::fromUtf8(encrypted.toBase64());
}




QString MainWindow::decryptMessageFromPeer(const QString& peerOnion, const QString& encryptedBase64)
{
    QByteArray encrypted = QByteArray::fromBase64(encryptedBase64.toUtf8());
    QByteArray decrypted;
    bool ok = crypt.decrypt(encrypted, decrypted);
    if (!ok) {
        return QString();
    }

    return QString::fromUtf8(decrypted);
}
\


void MainWindow::onEncryptToggled(bool checked) {
    encryptCheckBox->blockSignals(true);
    encryptCheckBox->setChecked(!checked);

    const QString peerOnion = contactAddressLabel->text().trimmed();

    if (peerOnion.isEmpty() || peerOnion == myOnion) {
        QMessageBox::warning(this, "Invalid Peer", "Select a valid peer first");
        encryptCheckBox->blockSignals(false);
        return;
    }

    if (!crypt.keysExist()) {
        QMessageBox::warning(this, "No Keys", "Generate your encryption keys first");
        encryptCheckBox->blockSignals(false);
        return;
    }

    const QString peerPubKey = contactManager->getContact(peerOnion).publicKey;
    if (peerPubKey.isEmpty()) {
        QMessageBox::warning(this, "No Public Key",
                             "This contact has no public key");
        encryptCheckBox->blockSignals(false);
        return;
    }

    const QString trimmedKey = peerPubKey.trimmed();
    if (!trimmedKey.startsWith("-----BEGIN PUBLIC KEY-----") ||
        !trimmedKey.endsWith("-----END PUBLIC KEY-----")) {
        QMessageBox::warning(this, "Invalid Key Format",
                             "Public key must be in PEM format");
        encryptCheckBox->blockSignals(false);
        return;
    }

    encryptCheckBox->setChecked(checked);
    encryptCheckBox->blockSignals(false);

    Contact contact = contactManager->getContact(peerOnion);
    contact.encryptionEnabled = checked;
    contactManager->updateContact(contact);
}



void MainWindow::exportMyInfo()
{
    QDialog* dialog = new QDialog(this);
    dialog->setWindowTitle(tr("Export My Contact Info"));

    QFormLayout* form = new QFormLayout(dialog);

    QLineEdit* nameInput = new QLineEdit(dialog);
    nameInput->setPlaceholderText(tr("**Required"));

    QLineEdit* onionInput = new QLineEdit(dialog);
    onionInput->setPlaceholderText(tr("**Required"));

    QTextEdit* publicKeyInput = new QTextEdit(dialog);
    publicKeyInput->setPlaceholderText(tr("Recommended: Paste your public key here"));

    QLineEdit* commentsInput = new QLineEdit(dialog);
    commentsInput->setPlaceholderText(tr("Optional: Add comments or notes"));

    QString myOnion;
    QFile onionFile(getTorChatHiddenDirPath() + "/hostname");
    if (onionFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        myOnion = QString::fromUtf8(onionFile.readAll()).trimmed();
        onionFile.close();
    }
    onionInput->setText(myOnion);

    QString pubKey = crypt.getPublicKey();
    if (!pubKey.isEmpty()) {
        publicKeyInput->setPlainText(pubKey);
    }else{
        QMessageBox::StandardButton reply = QMessageBox::warning(
            this,
            tr("No Key Found"),
            tr("No keypair found. It is highly advisable to create a keypair before exporting your contact info.\n\nProceed without keypair?"),
            QMessageBox::Yes | QMessageBox::No
            );
        if (reply == QMessageBox::Yes) {
        } else {
            return;
        }

    }

    form->addRow(tr("Name:"), nameInput);
    form->addRow(tr("Onion Address:"), onionInput);
    form->addRow(tr("Public Key:"), publicKeyInput);
    form->addRow(tr("Comments:"), commentsInput);

    QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, dialog);
    form->addRow(buttonBox);

    connect(buttonBox, &QDialogButtonBox::accepted, dialog, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, dialog, &QDialog::reject);

    connect(dialog, &QDialog::accepted, this, [=]() {
        QString name = nameInput->text().trimmed();
        QString onion = onionInput->text().trimmed();
        QString pubKey = publicKeyInput->toPlainText().trimmed();
        QString comments = commentsInput->text().trimmed();

        if (name.isEmpty() || onion.isEmpty()) {
            QMessageBox::warning(dialog, tr("Missing Fields"), tr("Name and onion address are mandatory."));
            return;
        }

        QJsonObject obj;
        obj["name"] = name;
        obj["onion"] = onion;
        if (!pubKey.isEmpty())
            obj["public_key"] = pubKey;
        if (!comments.isEmpty())
            obj["comments"] = comments;

        QString baseName = name;
        baseName.remove(' '); // Remove spaces
        QString filePath = QDir(getContactsDirPath()).filePath(baseName + ".contact");

        QFile file(filePath);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            QMessageBox::warning(dialog, tr("Export Failed"), tr("Could not write to file: %1").arg(filePath));
            return;
        }
        QJsonDocument doc(obj);
        file.write(doc.toJson(QJsonDocument::Indented));
        file.close();

        QMessageBox::information(dialog, tr("Export Successful"), tr("Contact info exported to:\n%1").arg(filePath));
    });

    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->exec();
}


void MainWindow::checkAndUpdateContacts()
{
    if (!networkManager->getConnectedPeers().isEmpty()) {
        QMessageBox::StandardButton reply = QMessageBox::question(
            this,
            tr("Active Connections"),
            tr("You have open connections. This may disconnect you. Continue?"),
            QMessageBox::Yes | QMessageBox::No
            );

        if (reply == QMessageBox::No) return;
    }

    updateContactsList();

}

void MainWindow::editTorrcPortsDialog()
{
    QString torrcPath = getTorrcDirPath();
    QFile file(torrcPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "Error", "Cannot open torrc file.");
        return;
    }

    int port1 = 8080, port2 = 9090, controlPort = 9051, proxyPort = 9050;

    QRegularExpression hsPortRe(R"(^HiddenServicePort\s+\d+\s+127\.0\.0\.1:(\d+))");
    QRegularExpression controlPortRe(R"(^ControlPort\s+(\d+))");
    QRegularExpression socksPortRe(R"(^SocksPort\s+(\d+))");

    QTextStream in(&file);
    int found = 0;
    while (!in.atEnd()) {
        QString line = in.readLine();
        QRegularExpressionMatch m = hsPortRe.match(line);
        if (m.hasMatch()) {
            if (found == 0) port1 = m.captured(1).toInt();
            else if (found == 1) port2 = m.captured(1).toInt();
            found++;
        }
        m = controlPortRe.match(line);
        if (m.hasMatch()) {
            controlPort = m.captured(1).toInt();
        }
        m = socksPortRe.match(line);
        if (m.hasMatch()) {
            proxyPort = m.captured(1).toInt();
        }
    }
    file.close();

    QDialog dialog(this);
    dialog.setWindowTitle("Edit Torrc Inner Ports");
    QFormLayout *layout = new QFormLayout(&dialog);

    QString defaultChatPort = "8080";
    QString defaultHttpPort = "9090";
    QString defaultControlPort = "9051";
    QString defaultProxyPort = "9050";

    QLineEdit *currentChatPort = new QLineEdit(QString::number(port1));
    QLineEdit *currentHttpPort = new QLineEdit(QString::number(port2));
    QLineEdit *currentControlPort = new QLineEdit(QString::number(controlPort));
    QLineEdit *currentProxyPort = new QLineEdit(QString::number(proxyPort));

    currentChatPort->setReadOnly(true);
    currentHttpPort->setReadOnly(true);
    currentControlPort->setReadOnly(true);
    currentProxyPort->setReadOnly(true);

    QLineEdit *editChatPort = new QLineEdit(QString::number(port1));
    QLineEdit *editHttpPort = new QLineEdit(QString::number(port2));
    QLineEdit *editControlPort = new QLineEdit(QString::number(controlPort));
    QLineEdit *editproxyPort = new QLineEdit(QString::number(proxyPort));

    layout->addRow("Current ChatServicePort :", currentChatPort);
    layout->addRow("New ChatServicePort :", editChatPort);
    layout->addRow("Current HttpServicePort :", currentHttpPort);
    layout->addRow("New HttpServicePort :", editHttpPort);
    layout->addRow("Current ControlPort:", currentControlPort);
    layout->addRow("New ControlPort:", editControlPort);
    layout->addRow("Current ProxyPort:", currentProxyPort);
    layout->addRow("New ProxyPort:", editproxyPort);
    QPushButton *restoreDefaultsBtn = new QPushButton("Restore Defaults", &dialog);
    layout->addWidget(restoreDefaultsBtn);
    QObject::connect(restoreDefaultsBtn, &QPushButton::clicked, [&]() {
        editChatPort->setText(defaultChatPort);
        editHttpPort->setText(defaultHttpPort);
        editControlPort->setText(defaultControlPort);
        editproxyPort->setText(defaultProxyPort);

    });

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel, &dialog);
    layout->addWidget(buttonBox);

    QObject::connect(buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    QObject::connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    if (dialog.exec() != QDialog::Accepted)
        return;


    bool ok1, ok2, ok3, ok4;
    int newChatPort = editChatPort->text().toInt(&ok1);
    int newHttpPort = editHttpPort->text().toInt(&ok2);
    int newControlPort = editControlPort->text().toInt(&ok3);
    int newProxyPort = editproxyPort->text().toInt(&ok4);

    if (!ok1 || !ok2 || !ok3 || !ok4) {
        QMessageBox::warning(this, "Invalid Input", "Please enter valid port numbers.");
        return;
    }

/*
if (newChatPort == newHttpPort ||
    newChatPort == newControlPort ||
    newChatPort == newProxyPort ||
    newHttpPort == newControlPort ||
    newHttpPort == newProxyPort ||
    newControlPort == newProxyPort)
{
    QMessageBox::warning(this, "Port Conflict", "All ports must be different.");
    return;
}
*/

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "Error", "Cannot re-open torrc file.");
        return;
    }
    QStringList lines;
    QTextStream in2(&file);
    found = 0;
    while (!in2.atEnd()) {
        QString line = in2.readLine();
        QRegularExpressionMatch m = hsPortRe.match(line);
        if (m.hasMatch()) {
            if (found == 0)
                line.replace(m.captured(1), QString::number(newChatPort));
            else if (found == 1)
                line.replace(m.captured(1), QString::number(newHttpPort));
            found++;
        }
        m = controlPortRe.match(line);
        if (m.hasMatch()) {
            line.replace(m.captured(1), QString::number(newControlPort));
        }
        m = socksPortRe.match(line);
        if (m.hasMatch()) {
            line.replace(m.captured(1), QString::number(newProxyPort));
        }
        lines << line;
    }
    file.close();

    if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
        QMessageBox::warning(this, "Error", "Cannot write to torrc file.");
        return;
    }
    QTextStream out(&file);
    for (const QString &line : lines) {
        out << line << "\n";
    }
    file.close();

    QMessageBox::information(this, "Success", "Ports updated in torrc.\n"
                                              "Please restart the application so that the changes take effect.");
}



void MainWindow::loadPortsFromTorrc() {
    QString torrcPath = getTorrcDirPath();
    QFile file(torrcPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return;

    QRegularExpression hsPortRe(R"(^HiddenServicePort\s+\d+\s+127\.0\.0\.1:(\d+))");
    QRegularExpression controlPortRe(R"(^ControlPort\s+(\d+))");
    QRegularExpression socksPortRe(R"(^SocksPort\s+(\d+))");

    QTextStream in(&file);
    int found = 0;
    while (!in.atEnd()) {
        QString line = in.readLine();
        QRegularExpressionMatch m = hsPortRe.match(line);
        if (m.hasMatch()) {
            if (found == 0) DefaultChatServicePort = m.captured(1).toInt();
            else if (found == 1) DefaultHttpServicePort = m.captured(1).toInt();
            found++;
        }
        m = controlPortRe.match(line);
        if (m.hasMatch()) {
            DefaultControlPort = m.captured(1).toInt();
        }
        m = socksPortRe.match(line);
        if (m.hasMatch()) {
            DefaultProxyPort = m.captured(1).toInt();
        }
    }
    file.close();
}


void MainWindow::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasUrls()) {
        const QList<QUrl> urls = event->mimeData()->urls();
        for (const QUrl &url : urls) {
            if (url.isLocalFile() && url.toLocalFile().endsWith(".contact", Qt::CaseInsensitive)) {
                event->acceptProposedAction();
                return;
            }
        }
    }
    event->ignore();
}

void MainWindow::dropEvent(QDropEvent *event)
{
    if (event->mimeData()->hasUrls()) {
        const QList<QUrl> urls = event->mimeData()->urls();
        for (const QUrl &url : urls) {
            if (url.isLocalFile() && url.toLocalFile().endsWith(".contact", Qt::CaseInsensitive)) {
                QString filePath = url.toLocalFile();

                QFile file(filePath);
                if (!file.open(QIODevice::ReadOnly)) {
                    QMessageBox::warning(this, tr("Import Failed"), tr("Could not open contact file: %1").arg(filePath));
                    continue;
                }

                QByteArray data = file.readAll();
                file.close();

                QJsonParseError parseError;
                QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);

                if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
                    QMessageBox::warning(this, tr("Import Failed"), tr("Invalid contact file format: %1").arg(filePath));
                    continue;
                }

                QJsonObject obj = doc.object();

                if (!obj.contains("onion") || !obj.contains("name")) {
                    QMessageBox::warning(this, tr("Import Failed"), tr("Contact file missing required fields: %1").arg(filePath));
                    continue;
                }

                addContact(obj.value("onion").toString(), obj);
            }
        }
        event->acceptProposedAction();
    } else {
        event->ignore();
    }
}


bool MainWindow::eventFilter(QObject* obj, QEvent* event)
{


    if (event->type() == QEvent::Wheel) {
        QWheelEvent* wheelEvent = static_cast<QWheelEvent*>(event);

        if (QApplication::keyboardModifiers() & Qt::ControlModifier) {

            if (wheelEvent->angleDelta().y() > 0)
                m_bubbleFontSize = std::min(m_bubbleFontSize + 1, 32);
            else if (wheelEvent->angleDelta().y() < 0)
                m_bubbleFontSize = std::max(m_bubbleFontSize - 1, 8);

            QWidget* currentTab = chatTabWidget->currentWidget();
            if (!currentTab) return true;
            QScrollArea* scrollArea = currentTab->findChild<QScrollArea*>();
            if (!scrollArea) return true;
            QWidget* chatWidget = scrollArea->widget();
            if (!chatWidget) return true;
            QVBoxLayout* layout = qobject_cast<QVBoxLayout*>(chatWidget->layout());
            if (!layout) return true;

            for (int i = 0; i < layout->count(); ++i) {
                QWidget* w = layout->itemAt(i)->widget();
                if (auto bubble = qobject_cast<MessageBubbleWidget*>(w)) {
                    bubble->setFontSize(m_bubbleFontSize);
                }
            }
            return true; // Event handled
        }
    }
    return QObject::eventFilter(obj, event);
}


void MainWindow::setupMainLayoutWithSplitter() {
    QSplitter* splitter = new QSplitter(this);
    splitter->setOrientation(Qt::Horizontal);
    splitter->setHandleWidth(4);  // Increased from default (usually 4-5)

    splitter->setStyleSheet(
        "QSplitter::handle {"
        "   background-color: #e8d5b5;"  // Discreet yellow-beige
        "   width: 4px;"                  // Match handle width
        "}"
        "QSplitter::handle:hover {"
        "   background-color: #d4b68a;"   // Slightly darker on hover
        "}"
        "QSplitter::handle:pressed {"
        "   background-color: #c4a076;"   // Even darker when pressed
        "}"
    );
    setupDockWidgets(); // This creates contactsDock and dockContent

    setupChatArea(); // This creates centralWidget and its layout

    splitter->addWidget(contactsDock);      // Left side: contacts
    splitter->addWidget(centralWidget);     // Right side: chat area

    splitter->setStretchFactor(0, 0); // contactsDock
    splitter->setStretchFactor(1, 1); // chat area
    splitter->setSizes({300, 800});   // Initial sizes at startup


    setCentralWidget(splitter);
}


void MainWindow::checkFirstRun() {

    bool firstRun = settings.value("firstRun", true).toBool();

    if (firstRun) {
        settings.setValue("firstRun", false);

        QMessageBox::information(
            this,
            "Setup Complete",
            "To finalize setup, please restart the application "
            "after the UI is fully loaded.\n\n"
            "⚠ IMPORTANT: It is essential that you stop all other Tor "
            "applications before running Selene, or the app will not "
            "run properly.\n\n"
            "You can later edit the torrc ports from the app's Settings "
            "so that you can have more than one instance of Tor running. "
            "But for the time being, it is best to ensure no other Tor "
            "service is running.");
    }
}

void MainWindow::performFactoryReset() {

    QMessageBox::StandardButton reply = QMessageBox::critical(
        this,
        tr("Confirm Factory Reset"),
        tr("WARNING: This will delete ALL your application data, including contacts, chat history, settings, and all Tor hidden service keys.\n\n"
           "This action is irreversible.\n\n"
           "You will lose all your data and new onion addresses will be generated for all services after restart.\n\n"
           "Do you want to continue?"),
        QMessageBox::Yes | QMessageBox::No
        );
    if (reply != QMessageBox::Yes) return;
    QString appDataDir = getAppDataDir();
    QString configDir = getConfigDirPath();

    QDir dir(appDataDir);
    bool deleted = dir.exists() ? dir.removeRecursively() : false;

    QDir dirConfig(configDir);
    bool deletedConfig = dirConfig.exists() ? dirConfig.removeRecursively() : true;

    QFile configFile(getConfigDirPath() + ".conf");
    if (configFile.exists()) {
        if (!configFile.remove()) {
        } else {
        }
    }

    if (deleted && deletedConfig) {
        QMessageBox::information(
            this,
            tr("Factory Reset Complete"),
            tr("All application data has been deleted.\n"
               "Please restart the application to complete the factory reset.")
            );
        qApp->quit();

    } else {
        QMessageBox::warning(
            this,
            tr("Factory Reset Failed"),
            tr("Failed to delete some or all application data.\n"
               "Please check your permissions and try again.")
            );
    }

}

void MainWindow::updateBubbleFontSizes()
{
    QWidget* currentTab = chatTabWidget->currentWidget();
    if (!currentTab) return;

    QScrollArea* scrollArea = currentTab->findChild<QScrollArea*>();
    if (!scrollArea) return;

    QWidget* chatWidget = scrollArea->widget();
    if (!chatWidget) return;

    QVBoxLayout* layout = qobject_cast<QVBoxLayout*>(chatWidget->layout());
    if (!layout) return;

    for (int i = 0; i < layout->count(); ++i) {
        QWidget* w = layout->itemAt(i)->widget();
        if (auto bubble = qobject_cast<MessageBubbleWidget*>(w)) {
            bubble->setFontSize(m_bubbleFontSize);
        }
    }
}

void MainWindow::addFontSizeCombo(){

    fontSizeCombo = new QComboBox(this);
    fontSizeCombo->setObjectName("fontSizeCombo");
    fontSizeCombo->setToolTip("Set chat font size\n"
                              "You can also use CTRL + '=/-'");

    for (int i = 8; i <= 18; ++i) {
        fontSizeCombo->addItem(QString::number(i), i);
    }

    int savedFontSize = settings.value("chatFontSize", 11).toInt();
    int fontSizeIndex = fontSizeCombo->findData(savedFontSize);
    if (fontSizeIndex != -1) {
        fontSizeCombo->setCurrentIndex(fontSizeIndex);
    } else {
        fontSizeCombo->setCurrentIndex(fontSizeCombo->findData(11));
    }
    m_bubbleFontSize = fontSizeCombo->currentData().toInt();

    connect(fontSizeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int idx) {
                int selectedFontSize = this->fontSizeCombo->itemData(idx).toInt();
                this->m_bubbleFontSize = selectedFontSize;
                this->messageInput->setFontPointSize(selectedFontSize);
                QSettings settings;
                settings.setValue("chatFontSize", selectedFontSize);

                this->updateBubbleFontSizes();
            });


    toolbar->addWidget(fontSizeCombo);
}


QString MainWindow::encryptFileForPeer(const QString &filePath, const QString &peerOnion)
{
    QFileInfo fi(filePath);
    QString fileName = fi.fileName();

    QString peerPublicKey = contactManager->getContact(peerOnion).publicKey;
    if (peerPublicKey.isEmpty()) {
        qWarning() << "No public key for peer" << peerOnion;
        return QString();
    }

    QByteArray aesKey = crypt.generateAESKey(32);

    QByteArray fileData;
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) return QString();
    fileData = file.readAll();
    file.close();

    QByteArray iv;
    QByteArray encryptedFileData = crypt.encryptAES(fileData, aesKey, iv);
    if (encryptedFileData.isEmpty()) {
        qWarning() << "Failed to encrypt file with AES";
        return QString();
    }

    QByteArray encryptedAESKey;
    if (!crypt.encrypt(aesKey, peerPublicKey, encryptedAESKey)) {
        qWarning() << "Failed to encrypt AES key";
        return QString();
    }

    QString tempPath = QStandardPaths::writableLocation(QStandardPaths::TempLocation)
                       + QDir::separator() + fileName + ".encbundle";

    QSaveFile bundleFile(tempPath);
    if (!bundleFile.open(QIODevice::WriteOnly)) {
        qWarning() << "Failed to create bundle file:" << tempPath;
        return QString();
    }

    QDataStream out(&bundleFile);
    out.setVersion(QDataStream::Qt_6_0);

    out << quint32(encryptedAESKey.size());
    out.writeRawData(encryptedAESKey.constData(), encryptedAESKey.size());

    out << quint32(iv.size());
    out.writeRawData(iv.constData(), iv.size());

    out << quint32(encryptedFileData.size());
    out.writeRawData(encryptedFileData.constData(), encryptedFileData.size());

    if (!bundleFile.commit()) {
        qWarning() << "Failed to save bundle file";
        return QString();
    }

    return tempPath;
}



void MainWindow::createSymlink()
{
#ifdef FLATHUB_BUILD
    QString msg = "";
    QMessageBox msgBox;
    msgBox.setWindowTitle("Flatpak Permission Required");
    msgBox.setText(QString(
                       "%1 is running as a Flatpak and may not have access to your home directory.\n\n"
                       "To create a symlink, you may need to grant home directory access first.\n\n"
                       "Option 1 - Terminal:\n"
                       "  Grant access:\n"
                       "    flatpak override --user --filesystem=home io.github.alamahant.%1\n\n"
                       "  Revoke access later:\n"
                       "    flatpak override --user --nofilesystem=home io.github.alamahant.%1\n\n"
                       "Option 2 - Flatseal:\n"
                       "  Install Flatseal from Flathub and grant 'Home' access to %1.\n\n"
                       "If you have already granted permissions, you can continue."
                       ).arg(QApplication::applicationName()));

    msgBox.setIcon(QMessageBox::Information);

    QPushButton *continueButton = msgBox.addButton("Continue", QMessageBox::AcceptRole);
    QPushButton *cancelButton = msgBox.addButton("Cancel", QMessageBox::RejectRole);
    msgBox.setDefaultButton(cancelButton);

    msgBox.exec();

    if (msgBox.clickedButton() != continueButton) {
        return; // User cancelled
    }

#endif
    QString destinationDir = QFileDialog::getExistingDirectory(
                this,
                "Select Destination Folder for Symlink",
                QDir::homePath(),
                QFileDialog::ShowDirsOnly
                );

    if (destinationDir.isEmpty()) {
        return; // User cancelled
    }

    QString symlinkPath = QDir(destinationDir).filePath(QApplication::applicationName());
    if (QFile::exists(symlinkPath) || QFileInfo(symlinkPath).isSymLink()) {
        QMessageBox::StandardButton reply = QMessageBox::question(
                    this,
                    "Symlink Exists",
                    QString("A file or symlink already exists at:\n%1\n\nOverwrite?").arg(symlinkPath),
                    QMessageBox::Yes | QMessageBox::No
                    );

        if (reply != QMessageBox::Yes) {
            return;
        }

        if (!QFile::remove(symlinkPath)) {
            QMessageBox::warning(this, "Error", "Could not remove existing file/symlink");
            return;
        }
    }

    QString targetPath = getDocumentsDirPath();

    if (!QFile::exists(targetPath)) {
        QMessageBox::warning(this, "Error",
                             QString("Target directory does not exist:\n%1").arg(targetPath));
        return;
    }

    if (QFile::link(targetPath, symlinkPath)) {
        QMessageBox::information(
                    this,
                    "Symlink Created",
                    QString("Symlink created successfully!\n\n"
                            "Name: %3\n"
                            "Location: %1\n\n"
                            "Now you can access %3 data from:\n%2")
                    .arg(destinationDir)
                    .arg(symlinkPath)
                    .arg(QApplication::applicationName())
                    );
    } else {
        QMessageBox::warning(
                    this,
                    "Error",
                    QString("Failed to create symlink.\n\n"
                            "Destination: %1\n"
                            "Target: %2\n\n"
                            "Possible reasons:\n"
                            "• Insufficient permissions\n"
                            "• Invalid destination path\n"
                            "• Filesystem doesn't support symlinks")
                    .arg(symlinkPath)
                    .arg(targetPath)
                    );
    }
}

void MainWindow::openFolder() {
    QDir dir(getDocumentsDirPath());
    if (!dir.exists()) {
        return;
    }

    if (!QDesktopServices::openUrl(QUrl::fromLocalFile(getDocumentsDirPath()))) {
    }
}

void MainWindow::setupCornerWidget()
{


    QPushButton *infoButton = new QPushButton("ⓘ", this);
    infoButton->setFixedSize(24, 24);
    infoButton->setToolTip(
        "<html><body style='white-space: nowrap;'>"
        "<b>Selene — Quick Guide</b><br><br>"

        "<b>Getting Started</b><br>"
        "• Restart after first launch to finalize setup<br>"
        "• Tor starts automatically, generating your .onion addresses<br>"
        "• Left panel: contacts · Right panel: chat<br><br>"

        "<b>Contacts</b><br>"
        "• '+ Add Contact' or drag &amp; drop a .contact file<br>"
        "• Onion address and name are required<br>"
        "• Search: [text] name · #b blocked · #c comments · #g group · #o onion<br>"
        "• Right-click a contact to Edit, Block, or Delete<br><br>"

        "<b>Connecting &amp; Chatting</b><br>"
        "• Enter peer's onion address → Connect<br>"
        "* Or select a contact from the lsit<br>"
        "• 'Open Chat' opens a tab without connecting<br>"
        "• Send: button or Ctrl+Enter<br>"
        "• Encrypt checkbox → RSA-encrypted(up to RSA 8192 bit) message<br><br>"

        "• Emoji: 😊 button · Font size: Ctrl+= / Ctrl+-<br><br>"

        "<b>Sending Files</b><br>"
        "• Connect first, then 'Send File'<br>"
        "• Files served via your HTTP onion; URL sent to peer<br>"
        "• Encrypt checkbox → AES encrypted bundle<br><br>"

        "<b>Encryption (RSA)</b><br>"
        "• Toolbar → Create New Key Pair (2048/4096/8192)<br>"
        "• Check 'Encrypt' to secure messages to a peer<br>"
        "• Peer needs a valid PEM public key in their contact<br>"
        "• Share yours via Copy Public Key or Export My Info<br><br>"

        "<b>Tor Menu</b><br>"
        "• New Tor Circuit<br>"
        "• New Chat/HTTP Onion (⚠ resets address, restart required)<br>"
        "• Edit Torrc Ports (only if ports clash with another Tor app)<br>"
        "• Bridge Manager for censorship circumvention<br><br>"

        "<b>HTTP Server</b><br>"
        "• Copy server onion / WWW dir<br>"
        "• Start · Stop · Restart<br>"
        "• Open HTTP Client<br><br>"

        "<b>Settings</b><br>"
        "• Factory Reset (⚠ deletes everything)<br>"
        "• Mute Sounds (Ctrl+M) · Enable Logger · View Log (Ctrl+L)<br>"
        "• Set Scale (Ctrl+Shift+S) · Set Font Size (Ctrl+Shift+F)<br><br>"

        "<b>Chat History</b><br>"
        "• Clear Current Chat / Clear All Chats<br>"
        "• Auto-saved; optional clear on exit<br><br>"

        "<b>Shortcuts</b><br>"
        "• Ctrl+Enter send · Ctrl+= / Ctrl+- font<br>"
        "• Ctrl+M mute · Ctrl+L log · Ctrl+Q quit<br>"
        "• Ctrl+Shift+S scale · Ctrl+Shift+F font size<br><br>"

        "<b>Help Menu</b><br>"
        "• About · Features · Changelog · Instructions · Support Us"

        "</body></html>"
    );
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

    infoButton->setObjectName("infoButton");


    QWidget *cornerContainer = new QWidget(this);
    QHBoxLayout *cornerLayout = new QHBoxLayout(cornerContainer);
    cornerLayout->setContentsMargins(0, 0, 6, 0);
    cornerLayout->setSpacing(6);
    cornerLayout->addWidget(infoButton);

    if (menubar) {
        menuBar()->setCornerWidget(cornerContainer, Qt::TopRightCorner);
    }
}
