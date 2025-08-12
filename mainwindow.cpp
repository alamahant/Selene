#include "mainwindow.h"
#include "./ui_mainwindow.h"


#include <QWidget>
#include <QDockWidget>
#include<QClipboard>
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

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , settings(QSettings())
{

    loadPortsFromTorrc();

    qDebug() << DefaultChatServicePort << " "<< DefaultHttpServicePort << " " << DefaultControlPort << " " <<DefaultProxyPort;
    Logger::log(Logger::INFO, QString("Default ports: Chat=%1, HTTP=%2, Control=%3, Proxy=%4")
                                  .arg(DefaultChatServicePort)
                                  .arg(DefaultHttpServicePort)
                                  .arg(DefaultControlPort)
                                  .arg(DefaultProxyPort));

    ui->setupUi(this);
    setAcceptDrops(true);

    setWindowTitle("Selene - P2P Chat");
    setWindowIcon(QIcon(":/icons/selene.png"));  // Resource or file path

    resize(1050, 700);
    isConnected = false;
    connectedPeerAddress = "";
    contactNameLabel = nullptr;
    contactAddressLabel = nullptr;
    contactCommentsLabel = nullptr;

    //Delete temp www dirs
    cleanTmpWWWDirs();

    // Initialize network components first
    if (torConfig.setupHiddenService()) {
        QString configPath = getAppDataDir() + "/.selene/torrc";
        if (TorProcess::startTor(configPath)) {
            torConfig.readOnionAddress();
            QString onion = torConfig.getOnionAddress();
            networkManager = new NetworkManager(this);
            networkManager->initialize(onion);
        }
    }

    contactManager = networkManager->getContactManager();

    // Setup the UI components
    //setupDockWidgets();
    //setupChatArea();
    setupMainLayoutWithSplitter();
    // Connect network signals

    //connect(networkManager, &NetworkManager::connectionStatusChanged,
      //      this, &MainWindow::handleConnectionStatus);
    connect(networkManager, &NetworkManager::connectionStatusChanged,
            this, [this](bool connected, const QString& peer) {
                handleConnectionStatus(connected, peer);
            });
    //connect(networkManager, SIGNAL(peerDisconnected()),
      //      this, SLOT(handlePeerDisconnected()));

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

    // Create chat manager last (after UI is set up)
    chatManager = new ChatManager(this);

    setupChatManager();
    connect(chatManager, &ChatManager::messageAdded,
            this, &MainWindow::handleMessageAdded);
    // Compose the full path for the chat history file
    QString appDataDir = getAppDataDir();

    QString historyFile = getChatHistoryFilePath();

    if (QFile::exists(historyFile)) {
        bool loaded = chatManager->loadFromFile(historyFile);
    } else {
        // do something
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

    // Application exit tasks
    connect(qApp, &QCoreApplication::aboutToQuit, this, &MainWindow::handleAppExit);
    // Http Server
    QString docRootPath = getWWWDir();
    //httpServer = new SimpleHttpFileServer(docRootPath, 9090, this);
    httpServer = new SimpleHttpFileServer(docRootPath, DefaultHttpServicePort, this);

    //sound welcome
    QTimer::singleShot(1000, this, [this]() {
        playSound(SoundType::MessageReceived, play);
    });


    QFile onionFile(getTorChatHiddenDirPath() + "/hostname");
    if (onionFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        myOnion = QString::fromUtf8(onionFile.readAll()).trimmed();
        onionFile.close();
    }
    qDebug() << "my ONION "<< myOnion;

    //clear history
    if (enableClearAllChatsCheckBox->isChecked()) {
        clearAllHistory();
    }
    // mute sounds
    bool checked = muteSoundsCheckBox->isChecked();
    if(networkManager) { networkManager->setPlay(!checked); }


    // enable logger
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


    checkFirstRun();
}

void MainWindow::setupChatArea() {
    // Create central widget
    centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);
    QVBoxLayout* layout = new QVBoxLayout(centralWidget);

    // Chat header with contact details
    QWidget* headerWidget = new QWidget(centralWidget);
    headerWidget->setObjectName("chatHeaderWidget");
    headerWidget->setStyleSheet("#chatHeaderWidget { background-color: #f5f5f5; border-radius: 8px; }");
    headerWidget->setMinimumHeight(100);
    QVBoxLayout* headerLayout = new QVBoxLayout(headerWidget);

    // Contact name (large, bold)
    contactNameLabel = new QLabel("No contact selected", headerWidget);
    QFont nameFont = contactNameLabel->font();
    nameFont.setBold(true);
    nameFont.setPointSize(nameFont.pointSize() + 2);
    contactNameLabel->setFont(nameFont);

    // Contact address (with copy button)
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

    // Comments section
    QHBoxLayout* commentsLayout = new QHBoxLayout();
    commentsLayout->addWidget(new QLabel("Comments:", headerWidget));
    contactCommentsLabel = new QLabel("", headerWidget);
    contactCommentsLabel->setWordWrap(true);
    contactCommentsLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    commentsLayout->addWidget(contactCommentsLabel, 1);

    // Status indicator
    statusLabel = new QLabel("Mode: Tor", headerWidget);
    statusLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);

    QHBoxLayout* nameLayout = new QHBoxLayout();
    nameLayout->addWidget(contactNameLabel);
    //nameLayout->addWidget(contactStatusDot);
    nameLayout->addStretch();
    headerLayout->addLayout(nameLayout);
    headerLayout->addLayout(addressLayout);
    headerLayout->addLayout(commentsLayout);
    headerLayout->addWidget(statusLabel);

    // Connection controls
    QWidget* connectionWidget = new QWidget(centralWidget);
    QHBoxLayout* connectionLayout = new QHBoxLayout(connectionWidget);
    peerAddressInput = new QLineEdit(connectionWidget);
    peerAddressInput->setPlaceholderText("Enter peer's onion address");

    connectButton = new QPushButton("Connect", connectionWidget);
    // Open Chat button (new)
    openChatButton = new QPushButton("Open Chat", connectionWidget);
    openChatButton->setToolTip("Open chat tab without connecting");

    connectionLayout->addWidget(peerAddressInput);
    connectionLayout->addWidget(connectButton);
    connectionLayout->addWidget(openChatButton);

    // Create tab widget instead of single chat display
    chatTabWidget = new QTabWidget(centralWidget);
    chatTabWidget->setTabsClosable(true);

    connect(openChatButton, &QPushButton::clicked, this, [this]() {
        QString address = peerAddressInput->text().trimmed();
        if (address.isEmpty())
            return;
        // Open the chat tab without connecting
        openChatTabForPeer(address);
    });

    connect(chatTabWidget, &QTabWidget::tabCloseRequested, this, [this](int index) {
        QString onion = chatTabWidget->tabToolTip(index);
        chatDisplays.remove(onion);
        chatTabWidget->removeTab(index);
    });

    // Keep the original chatDisplay for compatibility with existing code
    // but don't add it directly to the layout
    chatDisplay = new QTextBrowser();

    // Message input area
    inputWidget = new QWidget(centralWidget);
    QHBoxLayout* inputLayout = new QHBoxLayout(inputWidget);
    messageInput = new QTextEdit(inputWidget);
    messageInput->setFixedHeight(50);
    // set font

    //
    messageInput->setPlaceholderText("Type your message...");
    sendButton = new QPushButton("Send", inputWidget);
    fileButton = new QPushButton("Send File", inputWidget);
    //encryption checkbox
    encryptCheckBox = new QCheckBox(inputWidget);
    encryptCheckBox->setToolTip("Encrypt messages to this peer");

    encryptCheckBox->setChecked(false); // Default: not checked
    connect(encryptCheckBox, &QCheckBox::toggled, this, &MainWindow::onEncryptToggled);


    // Add emoji button and dialog
    setupEmojiPickerDialog();

    inputLayout->addWidget(messageInput);
    inputLayout->addWidget(emojiButton);
    inputLayout->addWidget(sendButton);
    inputLayout->addWidget(fileButton);
    inputLayout->addWidget(encryptCheckBox);

    // Add all components to main layout
    layout->addWidget(headerWidget);
    layout->addWidget(connectionWidget);
    layout->addWidget(chatTabWidget, 1); // Give chat tab widget more stretch
    layout->addWidget(inputWidget);

    // Connect signals
    connect(sendButton, &QPushButton::clicked, this, &MainWindow::sendMessage);

    connect(fileButton, &QPushButton::clicked, this, &MainWindow::sendFile);
    connect(connectButton, &QPushButton::clicked, this, &MainWindow::connectToPeer);
    connect(chatTabWidget, &QTabWidget::currentChanged, this, &MainWindow::handleTabChanged);

    // Update status with onion address
    QString onion = torConfig.getOnionAddress();
    if (!onion.isEmpty()) {
        updateStatus("Connected as: " + onion);
    }
}


MainWindow::~MainWindow() {
    // Clean up network connections first
    if (networkManager) {
        networkManager->cleanup();
    }

    // Stop Tor process
    TorProcess::stopTor();
    if(TorProcess::torInstance){
        delete TorProcess::torInstance;
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
    // Delete UI
    delete ui;

}

void MainWindow::updateStatus(const QString& status) {
    statusLabel->setText(status);
}


void MainWindow::connectToPeer() {


    // This is a connect request
    QString address = peerAddressInput->text().trimmed();

    // Check if we're already connected to this peer
    if (networkManager->isPeerConnected(address)) {
        networkManager->setActiveConnection(address);  // Add this line

        // This is a disconnect request
        networkManager->disconnectFromPeer(address);
        return;
    }

    if (address.isEmpty()) {
        return;
    }

    // Check if we're already connected to this peer
    if (networkManager->isPeerConnected(address)) {
        QMessageBox::information(this, "Already Connected",
                                 "You are already connected to this peer. Switching to existing conversation.");
        // Just switch to the existing tab
        switchToTab(address);
        return;
    }



    // Add a small delay to ensure sockets are properly closed
    QApplication::processEvents();

    // Now try to connect
    statusBar()->showMessage("Connecting to peer. Please wait up to 20 seconds...", 6 * 1000);
    // Show tray notification
    Notification("Connecting", "Connecting to peer. Please wait up to 20 seconds...");



    if (networkManager->connectToPeer(address)) {
        // Connection successful, create a tab if it doesn't exist
        if (!chatDisplays.contains(address)) {
            createChatTab(address);

        } else {
            // If tab exists, just switch to it
            switchToTab(address);

        }

        // Create a chat session if it doesn't exist
        if (!chatManager->getAllSessionPeers().contains(address)) {
            QString peerName = networkManager->getFriendlyName(address);
            chatManager->createSession(address, peerName);
        }

        // Activate the session
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
    // Update UI to show connected state for this specific peer


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
    // Update status label
    statusLabel->setText("Connected");

    // Ensure we have a chat session for this peer
    if (!chatManager->getAllSessionPeers().contains(peerOnion)) {
        QString peerName = networkManager->getFriendlyName(peerOnion);
        chatManager->createSession(peerOnion, peerName);
    }

    // Activate the session
    chatManager->activateSession(peerOnion);

}


void MainWindow::onAddContactClicked() {

    addContact(QString());
}




void MainWindow::onContactSelected(const QString& onionAddress) {
    // Set the peer address input
    peerAddressInput->setText(onionAddress);

    // Update connection state for this specific peer
    bool isPeerConnected = networkManager->isPeerConnected(onionAddress);
    connectButton->setText(isPeerConnected ? "Disconnect" : "Connect");
    peerAddressInput->setEnabled(!isPeerConnected);

    // Get contact details from contact manager
    Contact contact = contactManager->getContact(onionAddress);

    // Update contact details in header
    if (!contact.friendlyName.isEmpty()) {
        contactNameLabel->setText(contact.friendlyName);
    } else {
        contactNameLabel->setText("Unknown Contact");
    }
    contactAddressLabel->setText(onionAddress);
    if (!contact.comments.isEmpty()) {
        contactCommentsLabel->setText(contact.comments);
    } else {
        contactCommentsLabel->setText("No comments");
    }

    // Update status
    /*
    if (!contact.friendlyName.isEmpty()) {
        statusLabel->setText("Selected contact: " + contact.friendlyName);
    } else {
        statusLabel->setText("Selected contact: " + onionAddress);
    }
    */
    statusLabel->setText(isPeerConnected ? "Connected" : "Disconnected");



    // Switch to the corresponding tab if it exists
    for (int i = 0; i < chatTabWidget->count(); i++) {
        if (chatTabWidget->tabToolTip(i) == onionAddress) {
            chatTabWidget->setCurrentIndex(i);
            break;
        }
    }
    contactListWidget->setSelectedContact(onionAddress);
    // Update encryption checkbox to match the contact's setting
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
    // Create contacts dock widget
    contactsDock = new QDockWidget(tr("Contacts"), this);
    contactsDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    contactsDock->setFeatures(QDockWidget::DockWidgetMovable);
    //contactsDock->setMinimumWidth(300);  // or whatever width works for your cards
    contactsDock->setMinimumWidth(40);  // or whatever width works for your cards

    // Create widget for dock content
    //QWidget* dockContent = new QWidget(contactsDock);
    //QVBoxLayout* dockLayout = new QVBoxLayout(dockContent);
    dockContent = new QWidget(contactsDock);
    dockLayout = new QVBoxLayout(dockContent);
    // Add search box
    searchContactsEdit = new QLineEdit(dockContent);
    searchContactsEdit->setPlaceholderText(tr("🔍 Search contacts"));
    dockLayout->addWidget(searchContactsEdit);

    // Add contacts list
    contactListWidget = new ContactListWidget(dockContent);
    dockLayout->addWidget(contactListWidget);

    // Add button for adding contacts
    QPushButton* addContactBtn = new QPushButton(tr("+ Add Contact"), dockContent);
    dockLayout->addWidget(addContactBtn);

    contactsDock->setWidget(dockContent);
    addDockWidget(Qt::LeftDockWidgetArea, contactsDock);

    // Connect signals
    connect(searchContactsEdit, &QLineEdit::textChanged,
            contactListWidget, &ContactListWidget::filterContacts);
    connect(addContactBtn, &QPushButton::clicked,
            this, &MainWindow::onAddContactClicked);
    connect(contactListWidget, &ContactListWidget::contactSelected,
            this, &MainWindow::onContactSelected);
    connect(contactListWidget, &ContactListWidget::connectRequested,
            this, &MainWindow::connectToContact);



    // Load contacts
    contactListWidget->refreshContacts(networkManager->getContacts());
}


void MainWindow::setupChatManager()
{
    // Connect signals
    connect(chatManager, &ChatManager::messageAdded,
            this, [this](const QString& peerAddress, const ChatMessage& message) {

                // If we have a tab for this peer, render all messages properly
                if (chatDisplays.contains(peerAddress)) {
                    QScrollArea* display = chatDisplays[peerAddress];  // Changed from QTextBrowser*
                    // add event filter for increasing font size
                    //display->viewport()->installEventFilter(this);
                    //qDebug() << "Event filter installed on" << display->viewport();



                    // Use the chat manager's scroll area rendering method
                    //chatManager->renderMessagesToScrollArea(peerAddress, display);  // method call to rerender all msgs
                    chatManager->renderMessageToScrollArea(peerAddress, display, message, m_bubbleFontSize);  // Changed method call for appending single msg

                    // Highlight tab if it's not the current one
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

    // Set the peer address
    peerAddressInput->setText(onionAddress);

    // Connect to the peer
    connectToPeer();

    // Create a chat session if connection successful
    if (isConnected) {
        QString peerName = networkManager->getFriendlyName(onionAddress);
        chatManager->createSession(onionAddress, peerName);
        chatManager->activateSession(onionAddress);
    }
}

void MainWindow::handleIncomingMessage(const QString& senderAddress, const QString& message) {
    // Get the actual sender address
    QString actualSenderAddress = senderAddress;

    // If sender address is empty, use the connected peer address as fallback
    if (actualSenderAddress.isEmpty()) {
        actualSenderAddress = networkManager->getConnectedPeerAddress();

        // If still empty, we can't process this message
        if (actualSenderAddress.isEmpty()) {
            return;
        }
    }

    // Ensure we have a chat session
    if (!chatManager->getAllSessionPeers().contains(actualSenderAddress)) {
        // Get friendly name if this is a known contact
        QString senderName = networkManager->getFriendlyName(actualSenderAddress);

        // If not a known contact, use the address as the name
        if (senderName.isEmpty()) {
            senderName = actualSenderAddress;
            // Optionally, you could prompt the user to add this contact
            emit unknownContactMessageReceived(actualSenderAddress);
        }

        // Create the session regardless
        chatManager->createSession(actualSenderAddress, senderName);
    }

    // Activate the session if not active
    if (chatManager->getActiveSessionPeer() != actualSenderAddress) {
        chatManager->activateSession(actualSenderAddress);
    }

    // Add message to chat
    chatManager->addMessage(actualSenderAddress, message, false);
    //playSound(SoundType::MessageReceived);

}


void MainWindow::handleUnknownContact(const QString& senderAddress) {


    //playSound(SoundType::UnknownContact);
    createChatTab(senderAddress);
    switchToTab(senderAddress);
    // System tray notification
    Notification("Unknown Peer", "Received a message from: " + senderAddress);

    addContact(senderAddress);
}

void MainWindow::handleMessageAdded(const QString& peerAddress, const ChatMessage& message) {
    // If this is the active session, update the display
    if (chatManager->getActiveSessionPeer() == peerAddress) {
    }

}


void MainWindow::createChatTab(const QString& onionAddress) {
    // Check if tab already exists - KEEP original
    if (chatDisplays.contains(onionAddress)) {
        switchToTab(onionAddress);
        return;
    }

    // Get friendly name for the tab label - KEEP original
    QString friendlyName = networkManager->getFriendlyName(onionAddress);
    if (friendlyName.isEmpty() || friendlyName == onionAddress) {
        friendlyName = "Unknown (" + onionAddress.left(8) + "...)";
    }

    // Create a widget to hold the tab contents - KEEP original
    QWidget* tabWidget = new QWidget(chatTabWidget);
    chatTabWidgets[onionAddress] = tabWidget;


    QVBoxLayout* tabLayout = new QVBoxLayout(tabWidget);

    // Create a scroll area for this tab - CHANGED from QTextBrowser
    //QScrollArea* scrollArea = new QScrollArea(tabWidget);
    scrollArea = new QScrollArea(tabWidget);

    scrollArea->setWidgetResizable(true);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    scrollArea->setStyleSheet("QScrollArea { border: none; background-color: #f5f5f5; }");

    tabLayout->addWidget(scrollArea);

    // Store the scroll area in our map - CHANGED from textBrowser
    chatDisplays[onionAddress] = scrollArea;

    // Add the tab to the tab widget - KEEP original
    int index = chatTabWidget->addTab(tabWidget, friendlyName);
    chatTabWidget->setTabToolTip(index, onionAddress);

    // Switch to this tab - KEEP original
    chatTabWidget->setCurrentIndex(index);

    // Update header with contact info - KEEP original
    updateContactInfo(onionAddress);

    // CHANGED: Use ChatManager for rendering existing messages to scroll area
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

    // Find the tab index
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
    contactListWidget->onContactSelected();  // Add this after

    QDialog dialog(this);
    QFormLayout form(&dialog);

    Contact currentContact = contactManager->getContact(onion);

    QLineEdit* onionInput = new QLineEdit(currentContact.onionAddress, &dialog);
    onionInput->setReadOnly(true);
    QLineEdit* nameInput = new QLineEdit(currentContact.friendlyName, &dialog);
    //QTextEdit* publicKeyInput = new QTextEdit(currentContact.publicKey, &dialog);
    QTextEdit* publicKeyInput = new QTextEdit(&dialog);
    bool encryptionEnabled = currentContact.encryptionEnabled;
    QLineEdit* commentsInput = new QLineEdit(currentContact.comments, &dialog);

    form.addRow("Onion Address:", onionInput);
    form.addRow("Name:", nameInput);
    form.addRow("Public Key:", publicKeyInput);

    form.addRow("Comments:", commentsInput);

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

        //updatedContact.publicKey = publicKeyInput->toPlainText(); // text()
        updatedContact.comments = commentsInput->text();
        updatedContact.encryptionEnabled = encryptionEnabled;
        contactManager->updateContact(updatedContact);
        Notification("Contact Updated", "Updated contact: " + updatedContact.friendlyName);


        //checkAndUpdateContacts();
        updateContactsList();

        if (chatManager->getChatSessions().contains(onion)) {
            chatManager->getChatSessions()[onion].peerName = updatedContact.friendlyName;
        }        
    }
}



void MainWindow::addContact(const QString &sendersOnion, const QJsonObject &contact)
{
    // Reset last added contact name just to be safe
    lastAddedContactFriendlyName.clear();
    // Prevent multiple dialogs for the same onion address
    if (m_addContactDialogsOpen.contains(sendersOnion)) {
        return;
    }
    m_addContactDialogsOpen.insert(sendersOnion);


    // Create the dialog on the heap so it persists after this function returns
    QDialog* dialog = new QDialog(this);
    QFormLayout* form = new QFormLayout(dialog);

    QLineEdit* onionInput = new QLineEdit(dialog);
    QLineEdit* nameInput = new QLineEdit(dialog);
    QTextEdit* publicKeyInput = new QTextEdit(dialog);
    QLineEdit* commentsInput = new QLineEdit(dialog);



    onionInput->setMinimumWidth(70 * fontMetrics().horizontalAdvance('X'));
    onionInput->setPlaceholderText("**Required");
    nameInput->setMinimumWidth(70 * fontMetrics().horizontalAdvance('X'));
    nameInput->setPlaceholderText("**Required");

    publicKeyInput->setMinimumWidth(70 * fontMetrics().horizontalAdvance('X'));
    commentsInput->setMinimumWidth(70 * fontMetrics().horizontalAdvance('X'));

    form->addRow("Onion Address:", onionInput);
    form->addRow("Friendly Name:", nameInput);
    form->addRow("Public Key:", publicKeyInput);
    form->addRow("Comments:", commentsInput);
    // If called by handleunknown versus manually by clicking on buttgon
    if (!sendersOnion.isEmpty()) {
        onionInput->setText(sendersOnion);
    }

    // for enabling drop of .contact file
    if (!contact.isEmpty()) {
        if (contact.contains("onion"))
            onionInput->setText(contact.value("onion").toString());
        if (contact.contains("name"))
            nameInput->setText(contact.value("name").toString());
        if (contact.contains("public_key"))
            publicKeyInput->setPlainText(contact.value("public_key").toString());
        if (contact.contains("comments"))
            commentsInput->setText(contact.value("comments").toString());
    }


    //

    // --- Import Contact Button ---
    QPushButton* importContactBtn = new QPushButton("Import Contact", dialog);
    importContactBtn->setToolTip("Import Contact from a File");
    form->addRow(importContactBtn);

    connect(importContactBtn, &QPushButton::clicked, this, [=]() {
        QString filePath = QFileDialog::getOpenFileName(
            dialog,
            tr("Import Contact"),
            getContactsDirPath(), // Your function to get the app data directory
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

        // Validate required fields
        if (!obj.contains("onion") || !obj.contains("name")) {
            QMessageBox::warning(dialog, tr("Import Failed"), tr("Contact file missing required fields."));
            return;
        }

        // Populate fields
        onionInput->setText(obj.value("onion").toString());
        nameInput->setText(obj.value("name").toString());
        publicKeyInput->setPlainText(obj.value("public_key").toString());
        if (obj.contains("comments"))
            commentsInput->setText(obj.value("comments").toString());
        else
            commentsInput->clear();
    });


    QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, dialog);
    form->addRow(buttonBox);

    //connect(buttonBox, &QDialogButtonBox::accepted, dialog, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, dialog, &QDialog::reject);
    // Custom handler for Ok
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
        // All fields valid, accept dialog
        dialog->accept();
    });
    // Handle accepted signal non-modally
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
        contactManager->addContact(newContact);
        Notification("Contact Added", "Added contact: " + newContact.friendlyName);


        //checkAndUpdateContacts();
        updateContactsList();

        //lastAddedContactFriendlyName = newContact.friendlyName;
        if (chatManager->getChatSessions().contains(sendersOnion)) {
            chatManager->getChatSessions()[sendersOnion].peerName = newContact.friendlyName;
        }

        // Rename the tab here:
        QWidget* tabWidget = chatTabWidgets.value(sendersOnion, nullptr);
        if (tabWidget) {
            int idx = chatTabWidget->indexOf(tabWidget);
            if (idx != -1)
                chatTabWidget->setTabText(idx, lastAddedContactFriendlyName);
        }
    });

    // Clean up dialog if cancelled.--- not needed handled in the next connect
    //connect(dialog, &QDialog::rejected, dialog, &QDialog::deleteLater);

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
    //checkAndUpdateContacts();
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

        //checkAndUpdateContacts();
        updateContactsList();

    }
}


void MainWindow::handleTabChanged(int index) {
    if (index < 0) return;

    // Find which onion address corresponds to this tab - UPDATED for QScrollArea
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
        // Set this as the active connection - KEEP original
        networkManager->setActiveConnection(onionAddress);

        // Also update ChatManager's active session - KEEP original
        chatManager->activateSession(onionAddress);

        // Update contact info in the header - KEEP original
        updateContactInfo(onionAddress);

        // CHANGED: Re-render messages with scroll area formatting
        if (chatManager->getAllSessionPeers().contains(onionAddress)) {
            chatManager->renderMessagesToScrollArea(onionAddress, scrollArea, m_bubbleFontSize);  // CHANGED method and parameter
            chatManager->markAsRead(onionAddress);
        }

        // ADD: Clear tab highlight - KEEP original
        QString friendlyName = networkManager->getFriendlyName(onionAddress);
        chatTabWidget->setTabText(index, friendlyName);

        // Update status dot based on connection status - KEEP original
        if (networkManager->isPeerConnected(onionAddress)) {
            //contactStatusDot->setStyleSheet("background-color: #4CAF50; border-radius: 8px;"); // Green dot
        } else {
            //contactStatusDot->setStyleSheet("background-color: #BDBDBD; border-radius: 8px;"); // Gray dot
        }


    }
    peerAddressInput->setText(onionAddress);
    onContactSelected(onionAddress);
}


void MainWindow::sendMessage() {
    QString message = messageInput->toPlainText().trimmed();
    if (message.isEmpty()) return;

    // Get the current onion address - KEEP your original logic
    QString currentOnion = networkManager->getConnectedPeerAddress();
    if (currentOnion.isEmpty()) {
        QMessageBox::warning(this, "Cannot Send", "No active connection.");
        return;
    }

    //encrypt
    QString unencryptedMessage = message;
    if (encryptCheckBox->isChecked()) {
        // Use your helper to encrypt
        QString encrypted = encryptMessageForPeer(currentOnion, message);
        if (encrypted.isEmpty()) {
            QMessageBox::warning(this, "Encryption Error", "Failed to encrypt the message.");
            return;
        }
        message = encrypted;
    }
    //

    // Send the message - KEEP  original logic
        networkManager->sendMessage(message);

    // CHANGED: Always use ChatManager for proper rendering
    if (chatManager->getAllSessionPeers().contains(currentOnion)) {
        chatManager->addMessage(currentOnion, unencryptedMessage, true);
    } else {
        // Create session if it doesn't exist
        QString peerName = networkManager->getFriendlyName(currentOnion);
        chatManager->createSession(currentOnion, peerName);
        chatManager->addMessage(currentOnion, unencryptedMessage, true);
    }


    // Render after adding the message
    /*
    if (chatDisplays.contains(currentOnion)) {

        chatManager->renderMessagesToScrollArea(currentOnion, chatDisplays[currentOnion]);

    }


    if (chatDisplays.contains(currentOnion)) {
        QList<ChatMessage> messages = chatManager->getMessages(currentOnion);
        if (!messages.isEmpty()) {
            const ChatMessage& lastMsg = messages.last();
            chatManager->renderMessageToScrollArea(currentOnion, chatDisplays[currentOnion], lastMsg);
        }
    }
    */

    // scrol to the bottom
    QScrollBar* vScrollBar = scrollArea->verticalScrollBar();
    if (vScrollBar) {
        vScrollBar->setValue(vScrollBar->maximum());
    }
    // Clear input - KEEP your original logic

    messageInput->clear();
}

void MainWindow::updateContactInfo(const QString& onionAddress) {
    if (onionAddress.isEmpty()) {
        // No contact selected, clear the header
        contactNameLabel->setText("No contact selected");
        contactAddressLabel->setText("");
        contactCommentsLabel->setText("");
        //contactStatusDot->setStyleSheet("background-color: #BDBDBD; border-radius: 8px;"); // Light gray
        return;
    }

    // Get contact information from the contact manager
    QString friendlyName = networkManager->getFriendlyName(onionAddress);
    QString comments = "";
    // If we have a contact manager with additional info
    ContactManager* contactManager = networkManager->getContactManager();
    if (contactManager) {
        Contact contact = contactManager->getContact(onionAddress);
        if (!contact.comments.isEmpty()) {
            comments = contact.comments;
        }
    }

    // Update the header labels
    contactNameLabel->setText(friendlyName);
    contactAddressLabel->setText(onionAddress);
    contactCommentsLabel->setText(comments);

    // Check if this specific peer is connected
    if (networkManager->isPeerConnected(onionAddress)) {
        //contactStatusDot->setStyleSheet("background-color: #4CAF50; border-radius: 8px;"); // Green dot
    } else {
        //contactStatusDot->setStyleSheet("background-color: #F44336; border-radius: 8px;"); // Red dot
    }

    // Update window title to include contact name
    setWindowTitle("Selene - P2PChat - " + friendlyName);
}

void MainWindow::handleMessageReceived(const QString& senderOnion, const QString& message) {
    QString displayMessage = message;

    // Make sure we have a tab for this sender - KEEP your original logic
    if (!chatDisplays.contains(senderOnion)) {
        createChatTab(senderOnion);
    }

    // ADD: Ensure we have a chat session for proper HTML rendering
    if (!chatManager->getAllSessionPeers().contains(senderOnion)) {
        QString senderName = networkManager->getFriendlyName(senderOnion);
        if (senderName.isEmpty()) {
            senderName = senderOnion;
        }
        chatManager->createSession(senderOnion, senderName);
    }

    // ADD: Add message to ChatManager for HTML rendering
    //decrypt
    if (contactManager->getContact(senderOnion).encryptionEnabled) {
        QString decrypted = decryptMessageFromPeer(senderOnion, message);
        if (!decrypted.isEmpty()) {
            displayMessage = decrypted;
        } else {
            displayMessage = "[Failed to decrypt message]";
        }
    }
    //
    chatManager->addMessage(senderOnion, displayMessage, false);

    // Render after receiving the message
    /*
    if (chatDisplays.contains(senderOnion)) {
        chatManager->renderMessagesToScrollArea(senderOnion, chatDisplays[senderOnion]);

    }


    if (chatDisplays.contains(senderOnion)) {
        QList<ChatMessage> messages = chatManager->getMessages(senderOnion);
        if (!messages.isEmpty()) {
            const ChatMessage& lastMsg = messages.last();
            //chatManager->renderMessageToScrollArea(senderOnion, chatDisplays[senderOnion], lastMsg);
        }
    }
    */

    //playSound(SoundType::MessageReceived);

    // KEEP your original tab highlighting logic
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
        // You could change the tab text color or add an icon to indicate new messages
        chatTabWidget->setTabText(tabIndex, "* " + networkManager->getFriendlyName(senderOnion));
    }
    QScrollBar* vScrollBar = scrollArea->verticalScrollBar();
    if (vScrollBar) {
        vScrollBar->setValue(vScrollBar->maximum());
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

    toolbar->addSeparator();

    QLabel *httpServerLabel = new QLabel(this);
    httpServerLabel->setText("Server");
    toolbar->addWidget(httpServerLabel);
    toolbar->addAction(getHttpOnionAction);
    toolbar->addAction(getWWWDirAction);
    toolbar->addAction(startServerAction);
    toolbar->addAction(stopServerAction);
    toolbar->addAction(restartServerAction);
    //toolbar->addAction(newHttpOnionAction);

    toolbar->addSeparator();
    QLabel *chatLabel = new QLabel(this);
    chatLabel->setText("Chat");
    toolbar->addWidget(chatLabel);
    toolbar->addAction(copyOnionAction);

    toolbar->addAction(clearCurrentHistoryAction);
    toolbar->addAction(clearAllHistoryAction);
    //toolbar->addAction(newChatOnionAction);
    // Chat status combo box
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
    //add font size combo
    addFontSizeCombo();
    // export my info
    toolbar->addAction(exportMyInfoAction);

    toolbar->addSeparator();
    QLabel *encryptionLabel = new QLabel(this);
    encryptionLabel->setText("RSA");
    toolbar->addWidget(encryptionLabel);
    toolbar->addAction(copyPubKeyAction);
    toolbar->addAction(createKeypairAction);
    // Create rsa bits combo box and populate it
    rsaBitsCombo = new QComboBox(this);
    rsaBitsCombo->setObjectName("rsaBitsCombo");
    rsaBitsCombo->setToolTip("Select number of bits to be used with the new RSA key");
    rsaBitsCombo->addItem("2048", 2048);
    rsaBitsCombo->addItem("4096", 4096);
    rsaBitsCombo->addItem("8192", 8192);
    toolbar->addWidget(rsaBitsCombo);

    //reset
    //toolbar->addSeparator();
    //QLabel *resetLabel = new QLabel(this);
    //resetLabel->setText("Reset");
    //toolbar->addWidget(resetLabel);
    //toolbar->addAction(factoryResetAction);
    //Logger
    toolbar->addSeparator();
    QLabel *loggerLabel = new QLabel(this);
    loggerLabel->setText("Logger");
    toolbar->addWidget(loggerLabel);
    toolbar->addAction(toggleLoggingAction);
    toolbar->addAction(logViewerAction);
    toolbar->addSeparator();
    //toolbar->addAction(toggleMenuBarAction);
    toolbar->addAction(muteAction);

    return toolbar;
}


void MainWindow::handleAppExit()
{
    //QSettings settings;
    if (settings.value("autoClearHistoryOnExit", false).toBool()) {
        clearAllHistory();
    }
    chatManager->saveToFile(getChatHistoryFilePath());

    // Add other shutdown/cleanup tasks here as needed
}

void MainWindow::openChatTabForPeer(const QString& onionAddress) {
    if (!chatDisplays.contains(onionAddress)) {
        createChatTab(onionAddress);
    }
    switchToTab(onionAddress);


}

QStringList MainWindow::getAllEmojiResources() {
    // List of popular emoji Unicode characters
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
    // 1. Load emoji resources
    emojiPaths = getAllEmojiResources();

    // 2. Create the emoji picker widget (only once)
    emojiPicker = new EmojiPickerWidget(emojiPaths, this);

    // 3. Load the emoji font
    int fontId = QFontDatabase::addApplicationFont(":/emojis/NotoColorEmoji.ttf");
    if (fontId == -1) {
        qWarning() << "Failed to load NotoColorEmoji.ttf!";
    }
    QStringList loadedFamilies = QFontDatabase::applicationFontFamilies(fontId);
    if (!loadedFamilies.isEmpty()) {
    }

    // 4. Set the font for the input widget (messageInput)
    QFont font = messageInput->font();
    font.setFamily(font.family() + ", Noto Color Emoji");
    font.setPointSize(12); // Adjust as needed
    messageInput->setFont(font);

    // 5. Connect emoji picker selection to message input
    connect(emojiPicker, &EmojiPickerWidget::emojiSelected, this, [this](const QString& emoji) {
        QTextCursor cursor = messageInput->textCursor();
        cursor.insertText(emoji);
        messageInput->setTextCursor(cursor);

        // Optionally close the picker dialog if needed
        if (emojiPicker->parentWidget() && emojiPicker->parentWidget()->inherits("QDialog")) {
            emojiPicker->parentWidget()->close();
        }
    });

    // 6. Create the emoji button
    emojiButton = new QPushButton(inputWidget);
    emojiButton->setText("😊"); // Or set an icon if you prefer
    emojiButton->setFixedWidth(32);

    // 7. Create the picker dialog (only once)
    pickerDialog = new QDialog(nullptr);
    pickerDialog->setWindowFlags(Qt::Dialog);
    pickerDialog->setAttribute(Qt::WA_DeleteOnClose, false); // We want to reuse the dialog
    pickerDialog->setAttribute(Qt::WA_TranslucentBackground, true);

    // Style the dialog and buttons
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

    // Add scroll area for the emoji picker
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

    // Set a reasonable fixed size for the dialog
    pickerDialog->setFixedSize(350, 300);

    // 8. Connect emoji button to show the picker dialog
    connect(emojiButton, &QPushButton::clicked, this, [this]() {
        // Position the dialog below the emoji button
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
    // 1. Open file dialog in the "files" directory, allow multiple selection
    QString startDir = getFileSaveDirPath();
    QStringList filePaths = QFileDialog::getOpenFileNames(this, tr("Select File(s) to Share"), startDir);

    // 2. Check if user selected files
    if (filePaths.isEmpty())
        return;

    // 3. Create a UUID directory inside www
    QString wwwDir = getWWWDir();
    QString uuidDirName = QUuid::createUuid().toString(QUuid::WithoutBraces);
    QString destDirPath = wwwDir + QDir::separator() + uuidDirName;
    QDir().mkpath(destDirPath);

    // 4. Copy selected files into the UUID directory
    for (const QString& filePath : filePaths) {
        QFileInfo fileInfo(filePath);
        QString destFilePath = destDirPath + QDir::separator() + fileInfo.fileName();
        if (!QFile::copy(filePath, destFilePath)) {
            QMessageBox::warning(this, tr("File Copy Error"),
                                 tr("Failed to copy %1 to %2").arg(filePath, destFilePath));
            // Optionally: remove the directory and abort if any copy fails
            QDir(destDirPath).removeRecursively();
            return;
        }
    }

    // 5. Build the onion URL for the shared directory
    // Assume torConfig.getFileOnionAddress() returns something like "http://xxxx.onion"
    QString onionBase = torConfig.getFileOnionAddress();
    QString url = onionBase + "/" + uuidDirName + "/";

    // 6. Send the URL as a message using your existing sendMessage logic
    // Optionally, you can pre-fill the message input and call sendMessage()
    messageInput->setPlainText(url);
    sendMessage();

    // 7. Start/restart server
    if (httpServer->isRunning()) {
        httpServer->restart();
    }else{
        httpServer->start();
    }


    // Optionally, show a confirmation
    QMessageBox::information(this, tr("Files Shared"),
                             tr("Files have been shared at:\n%1").arg(url));
}

void MainWindow::playSound(SoundType type, bool playIt)
{
    if (!playIt) return;

    if (!m_soundEffects.contains(type)) {
        // Create and cache the sound effect for this type
        QSoundEffect* effect = new QSoundEffect(this);
        effect->setSource(QUrl("qrc" + soundResourcePath(type)));
        effect->setVolume(0.8);
        m_soundEffects[type] = effect;
    }
    QSoundEffect* effect = m_soundEffects[type];
    // If already playing, stop and replay
    if (effect->isPlaying()) {
        effect->stop();
    }
    effect->play();
}

/*
void MainWindow::clearCurrentTabHistory() {
    int index = chatTabWidget->currentIndex();
    if (index < 0) return;

    QString onion = chatTabWidget->tabToolTip(index);
    if (onion.isEmpty()) return;

    // Clear the chat display UI
    if (chatDisplays.contains(onion)) {
        //chatDisplays[onion]->clear();
    }

    // Remove from chatManager/history
    chatManager->clearHistoryForPeer(onion);
}
*/

void MainWindow::clearCurrentTabHistory() {
    int index = chatTabWidget->currentIndex();
    if (index < 0) return;

    QString onion = chatTabWidget->tabToolTip(index);
    if (onion.isEmpty()) return;

    // Clear the chat display UI
    if (chatDisplays.contains(onion)) {
        QScrollArea* scrollArea = chatDisplays[onion];
        QWidget* container = scrollArea->widget();
        if (container) {
            QLayout* layout = container->layout();
            if (layout) {
                // Remove and delete all message bubble widgets
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

    // Remove from chatManager/history
    chatManager->clearHistoryForPeer(onion);
}
/*
void MainWindow::clearAllHistory() {
    // Clear all chat displays
    for (auto display : chatDisplays) {
        //display->clear();
    }

    // Clear all history in chatManager
    chatManager->clearAllHistory();

    // Optionally, delete the global history file
    QFile::remove(getChatHistoryFilePath());
}
*/

void MainWindow::clearAllHistory() {
    // Clear all chat displays
    for (auto display : chatDisplays) {
        QScrollArea* scrollArea = display;
        QWidget* container = scrollArea->widget();
        if (container) {
            QLayout* layout = container->layout();
            if (layout) {
                // Remove and delete all message bubble widgets
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

    // Clear all history in chatManager
    chatManager->clearAllHistory();

    // Optionally, delete the global history file
    QFile::remove(getChatHistoryFilePath());
}

void MainWindow::setupMenuBar()
{
    // --- Tor Menu ---
    QMenu* torMenu = menubar->addMenu(tr("Tor"));
    newCircuitAction->setText(tr("New Tor Circuit"));
    torMenu->addAction(newCircuitAction);

    // --- HTTP Server Menu ---
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

    httpMenu->addSeparator();

    newHttpOnionAction->setText(tr("New HTTP Onion"));
    httpMenu->addAction(newHttpOnionAction);

    // --- Chat Menu ---
    QMenu* chatMenu = menubar->addMenu(tr("Chat"));
    clearCurrentHistoryAction->setText(tr("Clear Current Chat"));
    chatMenu->addAction(clearCurrentHistoryAction);

    clearAllHistoryAction->setText(tr("Clear All Chats"));
    chatMenu->addAction(clearAllHistoryAction);

    chatMenu->addSeparator();

    newChatOnionAction->setText(tr("New Chat Onion"));
    chatMenu->addAction(newChatOnionAction);

    // Add Copy Public Key and Create Key Pair actions to Chat menu
    copyPubKeyAction->setText(tr("Copy My Public Key"));
    chatMenu->addAction(copyPubKeyAction);

    createKeypairAction->setText(tr("Create New Key Pair"));
    chatMenu->addAction(createKeypairAction);

    // Add Export My Info action if not already present
    exportMyInfoAction->setText(tr("Export My Info"));
    chatMenu->addAction(exportMyInfoAction);

    // Add Copy Onion Address action if not already present
    copyOnionAction->setText(tr("Copy Chat Onion URL"));
    chatMenu->addAction(copyOnionAction);

    // --- Settings Menu ---
    QMenu* settingsMenu = menubar->addMenu(tr("Settings"));
    factoryResetAction->setText(tr("Factory Reset"));
    settingsMenu->addAction(factoryResetAction);

    settingsMenu->addSeparator();

    //toggleMenuBarAction->setText(tr("Show/Hide Menu Bar"));
    //settingsMenu->addAction(toggleMenuBarAction);

    // --- Add Checkboxes for Settings ---
    // QWidgetAction allows embedding widgets (like QCheckBox) in menus

    // 1. Enable/Disable "Clear All Chats"
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

    // Read from QSettings

    bool muteEnabled = settings.value("muteSoundsEnabled", false).toBool();
    muteSoundsCheckBox->setChecked(muteEnabled);
    muteAction->setChecked(muteEnabled);

    muteSoundsWidgetAction->setDefaultWidget(muteSoundsCheckBox);
    settingsMenu->addAction(muteSoundsWidgetAction);

    connect(muteSoundsCheckBox, &QCheckBox::toggled, this, [this](bool checked) {

        settings.setValue("muteSoundsEnabled", checked);
        muteAction->setChecked(checked);
    });

    // --- Enable/Disable Logger ---
    QWidgetAction* enableLoggerWidgetAction = new QWidgetAction(this);
    enableLoggerCheckBox = new QCheckBox(tr("Enable Logger"));

    // Read from QSettings
    bool loggerEnabled = settings.value("loggerEnabled", true).toBool();
    enableLoggerCheckBox->setChecked(loggerEnabled);
    toggleLoggingAction->setChecked(loggerEnabled);

    enableLoggerWidgetAction->setDefaultWidget(enableLoggerCheckBox);
    settingsMenu->addAction(enableLoggerWidgetAction);

    connect(enableLoggerCheckBox, &QCheckBox::toggled, this, [this](bool checked) {

        settings.setValue("loggerEnabled", checked);
        toggleLoggingAction->setChecked(checked);
    });




    // --- Security Menu (if present) ---
    if (securityManager) {
        securityManager->setupSecurityMenu(menubar);
    }

    // --- Log Viewer (if not already present) ---
    // You may want to add a Log Viewer action to the Settings menu
    if (logViewerAction && !settingsMenu->actions().contains(logViewerAction)) {
        logViewerAction->setText(tr("View Log"));
        settingsMenu->addAction(logViewerAction);
    }

    // --- Edit Torrc Ports (if not already present) ---
    if (editTorrcPortsAction && !settingsMenu->actions().contains(editTorrcPortsAction)) {
        editTorrcPortsAction->setText(tr("Edit Torrc Ports"));
        settingsMenu->addAction(editTorrcPortsAction);
    }

    QMenu* helpMenu = menubar->addMenu(tr("Help"));

    QAction* aboutAction = new QAction(tr("About"), this);
    helpMenu->addAction(aboutAction);

    // 2. Connect the About action to show the HelpMenuDialog
    connect(aboutAction, &QAction::triggered, this, [this]() {
        HelpMenuDialog dialog(HelpType::About, this);
        dialog.exec();
    });

    QAction* featuresAction = new QAction(tr("Features"), this);
    helpMenu->addAction(featuresAction);

    // 2. Connect the About action to show the HelpMenuDialog
    connect(featuresAction, &QAction::triggered, this, [this]() {
        HelpMenuDialog dialog(HelpType::Features, this);
        dialog.exec();
    });

    QAction* instructionsAction = new QAction(tr("Instructions"), this);
    helpMenu->addAction(instructionsAction);

    // 2. Connect the About action to show the HelpMenuDialog
    connect(instructionsAction, &QAction::triggered, this, [this]() {
        HelpMenuDialog dialog(HelpType::Instructions, this);
        dialog.exec();
    });

}

void MainWindow::setupActions()
{
    // Use style for standard icons
    QStyle* style = this->style();

    // New Tor Circuit
    //newCircuitAction = new QAction(style->standardIcon(QStyle::SP_BrowserReload), tr(""), this);
    newCircuitAction = new QAction(QIcon(":/icons/refresh-cw.svg"), tr(""), this);

    newCircuitAction->setObjectName("newcircuit");
    newCircuitAction->setToolTip(tr("Request a new Tor circuit"));
    connect(newCircuitAction, &QAction::triggered, this, []() {
        TorProcess::torInstance->newTorCircuit();
    });

    // Copy Server Onion
    //getHttpOnionAction = new QAction(style->standardIcon(QStyle::SP_FileDialogContentsView), tr(""), this);
    getHttpOnionAction = new QAction(QIcon(":/icons/copy.svg"), tr(""), this);

    getHttpOnionAction->setObjectName("gethttponion");
    getHttpOnionAction->setToolTip(tr("Copy HTTP server onion address"));
    connect(getHttpOnionAction, &QAction::triggered, this, [this]() {
        QString onionAddress = torConfig.getFileOnionAddress();
        QClipboard* clipboard = QGuiApplication::clipboard();
        clipboard->setText(onionAddress);
        QMessageBox::information(this, tr("Onion Address Copied"),
                                 tr("The HTTP file-sharing onion address has been copied to your clipboard:\n%1").arg(onionAddress));
    });

    // Copy WWW Dir
    //getWWWDirAction = new QAction(style->standardIcon(QStyle::SP_DirIcon), tr(""), this);
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

    // Start HTTP Server
    //startServerAction = new QAction(style->standardIcon(QStyle::SP_MediaPlay), tr(""), this);
    startServerAction = new QAction(QIcon(":/icons/play.svg"), tr(""), this);

    startServerAction->setObjectName("startserver");
    startServerAction->setToolTip(tr("Start HTTP server"));
    connect(startServerAction, &QAction::triggered, this, [this]() {
        httpServer->start();
        QMessageBox::information(this, tr("HTTP Server started"), tr("The HTTP server has started\n"));
        Notification(tr("HTTP Server started"), tr("The HTTP server has started\n"));
    });

    // Stop HTTP Server
    //stopServerAction = new QAction(style->standardIcon(QStyle::SP_MediaStop), tr(""), this);
    stopServerAction = new QAction(QIcon(":/icons/stop-circle.svg"), tr(""), this);

    stopServerAction->setObjectName("stopserver");
    stopServerAction->setToolTip(tr("Stop HTTP server"));
    connect(stopServerAction, &QAction::triggered, this, [this]() {
        httpServer->stop();
        QMessageBox::information(this, tr("HTTP Server stopped"), tr("The HTTP server has stopped\n"));
        Notification(tr("HTTP Server stopped"), tr("The HTTP server has stopped\n"));
    });

    // Restart HTTP Server
    //restartServerAction = new QAction(style->standardIcon(QStyle::SP_BrowserReload), tr(""), this);
    restartServerAction = new QAction(QIcon(":/icons/refresh-cw.svg"), tr(""), this);

    restartServerAction->setObjectName("restartserver");
    restartServerAction->setToolTip(tr("Restart HTTP server"));
    connect(restartServerAction, &QAction::triggered, this, [this]() {
        httpServer->restart();
        QMessageBox::information(this, tr("HTTP Server restarted"), tr("The HTTP server has restarted\n"));
        Notification(tr("HTTP Server restarted"), tr("The HTTP server has restarted\n"));

    });

    // Clear Current Chat
    //clearCurrentHistoryAction = new QAction(style->standardIcon(QStyle::SP_TrashIcon), tr(""), this);
    clearCurrentHistoryAction = new QAction(QIcon(":/icons/trash.svg"), tr(""), this);

    clearCurrentHistoryAction->setObjectName("clearcurrentchat");
    clearCurrentHistoryAction->setToolTip(tr("Clear current chat history"));
    // Read enabled/disabled state from QSettings


    connect(clearCurrentHistoryAction, &QAction::triggered, this, [this]() {
        clearCurrentTabHistory();
        QMessageBox::information(this, tr("Chat Cleared"), tr("Current chat history has been cleared."));
    });

    // Clear All Chats
    //clearAllHistoryAction = new QAction(style->standardIcon(QStyle::SP_TrashIcon), tr(""), this);
    clearAllHistoryAction = new QAction(QIcon(":/icons/trash-2.svg"), tr(""), this);

    clearAllHistoryAction->setObjectName("clearallchats");
    clearAllHistoryAction->setToolTip(tr("Clear all chat histories"));
    connect(clearAllHistoryAction, &QAction::triggered, this, [this]() {
        clearAllHistory();
        QMessageBox::information(this, tr("All Chats Cleared"), tr("All chat histories have been cleared."));
    });

    // New HTTP Onion
    //newHttpOnionAction = new QAction(style->standardIcon(QStyle::SP_FileDialogNewFolder), tr(""), this);
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

    // New Chat Onion
    //newChatOnionAction = new QAction(style->standardIcon(QStyle::SP_FileDialogNewFolder), tr(""), this);
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

    // Factory Reset
    //factoryResetAction = new QAction(style->standardIcon(QStyle::SP_DialogResetButton), tr(""), this);
    factoryResetAction = new QAction(QIcon(":/icons/rotate-ccw.svg"), tr(""), this);

    factoryResetAction->setObjectName("factoryreset");
    factoryResetAction->setToolTip(tr("Delete all app data and restore to factory settings"));
    connect(factoryResetAction, &QAction::triggered, this, &MainWindow::performFactoryReset);




    // Toggle Menu Bar
    //toggleMenuBarAction = new QAction(style->standardIcon(QStyle::SP_TitleBarMenuButton), tr(""), this);
    toggleMenuBarAction = new QAction(QIcon(":/icons/menu.svg"), tr(""), this);

    toggleMenuBarAction->setObjectName("togglemenubar");
    toggleMenuBarAction->setToolTip(tr("Show or hide the menu bar"));
    toggleMenuBarAction->setCheckable(true);
    toggleMenuBarAction->setChecked(menubar->isVisible());


    connect(toggleMenuBarAction, &QAction::toggled, this, [this](bool checked) {
        menubar->setVisible(checked);
    });


    // get public key
    //copyPubKeyAction = toolbar->addAction("Copy My Public Key");
    copyPubKeyAction = new QAction(QIcon(":/icons/clipboard.svg"), tr(""), this);

    copyPubKeyAction->setObjectName("copypublickey");
    copyPubKeyAction->setToolTip("Copy your public key to clipboard");

    connect(copyPubKeyAction, &QAction::triggered, this, [this]() {
        // Get the public key from your Crypto class
        QString pubKey = crypt.getPublicKey(); // Adjust if your method is named differently

        if (pubKey.isEmpty()) {
            QMessageBox::warning(this, "No Public Key", "You have not generated a key pair yet.");
            return;
        }

        QClipboard* clipboard = QGuiApplication::clipboard();
        clipboard->setText(pubKey);

        statusLabel->setText("Public key copied to clipboard");
    });
    // generate key pair
    //createKeypairAction = toolbar->addAction("Create New Key Pair");
    createKeypairAction = new QAction(QIcon(":/icons/key.svg"), tr(""), this);
    createKeypairAction->setObjectName("createnewkeypair");
    createKeypairAction->setToolTip("Generate a new public/private key pair (will overwrite existing keys)");

    connect(createKeypairAction, &QAction::triggered, this, [this]() {
        this->onCreateNewKeypair();
    });
    // export my info
    exportMyInfoAction = new QAction(QIcon(":/icons/file-text.svg"), tr(""), this);
    exportMyInfoAction->setObjectName("exportmyinfo");
    exportMyInfoAction->setToolTip("Export your contact information to a .contact file");

    connect(exportMyInfoAction, &QAction::triggered, this, [this]() {
        this->exportMyInfo();
    });
    // copy chat onion url
    copyOnionAction = new QAction(QIcon(":/icons/copy.svg"), tr(""), this);
    copyOnionAction->setObjectName("copyonionaddress");
    copyOnionAction->setToolTip("Copy your Chat service onion address to clipboard");

    // Connect the action
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
    //logging on/off
    toggleLoggingAction = new QAction(QIcon(":/icons/stop-circle.svg"), tr(""), this);
    toggleLoggingAction->setObjectName("togglelogging");
    toggleLoggingAction->setToolTip("Enable or disable application logging");
    toggleLoggingAction->setCheckable(true);
    toggleLoggingAction->setChecked(true); // Assume logging is enabled by default

    // Connect the toggled signal to enable/disable logging
    connect(toggleLoggingAction, &QAction::toggled, this, [this](bool checked) {
        Logger::setLoggingEnabled(checked);
        if (checked) {
            toggleLoggingAction->setIcon(QIcon(":/icons/stop-circle.svg"));
            toggleLoggingAction->setToolTip("Stop Logging");
            // Tray notification for logging enabled
            Notification("Logging Enabled", "Application logging is now enabled.");


            QMessageBox::information(nullptr, QObject::tr("Logging Enabled"), QObject::tr("Application logging is now enabled."));
        } else {
            toggleLoggingAction->setIcon(QIcon(":/icons/play-circle.svg"));
            toggleLoggingAction->setToolTip("Start Logging");

            Notification("Logging Disabled", "Application logging is now disabled.");


            //QMessageBox::information(nullptr, QObject::tr("Logging Disabled"), QObject::tr("Application logging is now disabled."));
        }
    });
    // edit torrc inner ports action
    editTorrcPortsAction = new QAction(QIcon(":/icons/edit.svg"), tr(""), this);
    editTorrcPortsAction->setObjectName("editTorrcPorts");
    editTorrcPortsAction->setToolTip(
        "Edit the inner ports in your torrc file.\n"
        "In most cases, you will never need to change these.\n"
        "An exception being if you are running another Tor-based app that uses the same ports.\n"
        "You will know this is necessary if Selene fails to create onion addresses for the Chat and HTTP services."
        );

    // Connect the action to your slot
    connect(editTorrcPortsAction, &QAction::triggered, this, [this]() {
        this->editTorrcPortsDialog();
    });
    // logviewer
    logViewerAction = new QAction(QIcon(":/icons/eye.svg"), tr(""), this);
    logViewerAction->setShortcut(QKeySequence("Ctrl+L"));
    connect(logViewerAction, &QAction::triggered, this, [this]() {
        QString logPath = getAppDataDir() + "/chat.log";
        LogViewerDialog dlg(logPath, this);

        // get state of toggle
        bool checked = toggleLoggingAction->isChecked();
        // stop logger
        toggleLoggingAction->setChecked(false);
        dlg.exec();
        // restore to initial state
        toggleLoggingAction->setChecked(checked);

    });
    // mute-unmute sounds
    // Create the Mute action
    muteAction = new QAction(QIcon(":/icons/volume-2.svg"), tr("Mute"), this);
    muteAction->setCheckable(true);
    muteAction->setChecked(false); // Start unchecked (not muted)

    // Optional: set a shortcut, e.g., Ctrl+M
    muteAction->setShortcut(QKeySequence("Ctrl+M"));

    // Connect the toggled signal to networkManager->setPlay
    connect(muteAction, &QAction::toggled, this, [this](bool checked) {
        // If checked, we want to mute, so setPlay(false)
        // If unchecked, unmute, so setPlay(true)
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
}


//crypto

void MainWindow::onCreateNewKeypair()
{
    // Ensure the combo box exists
    if (!rsaBitsCombo) {
        QMessageBox::critical(this, "Error", "RSA bits combo box not found.");
        return;
    }
    // get rsaBits from combobox
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
        // User confirmed, force overwrite
        if (crypt.generateKeyPair(rsaBits, true)) {
            QMessageBox::information(this, "Key Pair", "New key pair generated successfully.");
        } else {
            QMessageBox::critical(this, "Key Pair", "Failed to generate key pair.");
        }
        return;
    }

    // No keys exist, just generate
    if (crypt.generateKeyPair(rsaBits)) {
        QMessageBox::information(this, "Key Pair", "New key pair generated successfully.");
    } else {
        QMessageBox::critical(this, "Key Pair", "Failed to generate key pair.");
    }
}

QString MainWindow::encryptMessageForPeer(const QString& peerOnion, const QString& plainText)
{
    // Get the peer's public key
    QString peerPublicKey = contactManager->getContact(peerOnion).publicKey;

    // Debug: Show which peer and key we're using
    qDebug() << "[encryptMessageForPeer] Peer:" << peerOnion;
    qDebug() << "[encryptMessageForPeer] Public Key:" << peerPublicKey;

    QByteArray encrypted;

    // Crypto::encrypt returns bool, fills encrypted QByteArray
    bool ok = crypt.encrypt(plainText.toUtf8(), peerPublicKey, encrypted);

    if (!ok) {
        qDebug() << "[encryptMessageForPeer] Encryption failed!";
        qDebug() << "[encryptMessageForPeer] Plaintext:" << plainText;
        return QString();
    }

    // Debug: Show encrypted (base64) output
    qDebug() << "[encryptMessageForPeer] Encryption succeeded. Base64:" << QString::fromUtf8(encrypted.toBase64());

    // Return base64-encoded ciphertext for easy transport
    return QString::fromUtf8(encrypted.toBase64());
}




// Decrypt a message received from a peer (using our private key)
QString MainWindow::decryptMessageFromPeer(const QString& peerOnion, const QString& encryptedBase64)
{
    QByteArray encrypted = QByteArray::fromBase64(encryptedBase64.toUtf8());
    QByteArray decrypted;
    // Crypto::decrypt returns bool, fills decrypted QByteArray
    bool ok = crypt.decrypt(encrypted, decrypted);
    if (!ok) {
        // Handle decryption failure (could log or show error)
        return QString();
    }

    return QString::fromUtf8(decrypted);
}
\


void MainWindow::onEncryptToggled(bool checked) {
    // Lock UI and revert immediate change
    encryptCheckBox->blockSignals(true);
    encryptCheckBox->setChecked(!checked);

    // 1. Validate peer selection
    const QString peerOnion = contactAddressLabel->text().trimmed();
    qDebug() << "Toggling encryption for:" << peerOnion;

    if (peerOnion.isEmpty() || peerOnion == myOnion) {
        QMessageBox::warning(this, "Invalid Peer", "Select a valid peer first");
        encryptCheckBox->blockSignals(false);
        return;
    }

    // 2. Validate local crypto
    if (!crypt.keysExist()) {
        QMessageBox::warning(this, "No Keys", "Generate your encryption keys first");
        encryptCheckBox->blockSignals(false);
        return;
    }

    // 3. Validate peer's public key exists
    const QString peerPubKey = contactManager->getContact(peerOnion).publicKey;
    if (peerPubKey.isEmpty()) {
        QMessageBox::warning(this, "No Public Key",
                             "This contact has no public key");
        encryptCheckBox->blockSignals(false);
        return;
    }

    // 4. Validate key format
    const QString trimmedKey = peerPubKey.trimmed();
    if (!trimmedKey.startsWith("-----BEGIN PUBLIC KEY-----") ||
        !trimmedKey.endsWith("-----END PUBLIC KEY-----")) {
        QMessageBox::warning(this, "Invalid Key Format",
                             "Public key must be in PEM format");
        encryptCheckBox->blockSignals(false);
        return;
    }

    // ALL CHECKS PASSED - apply changes
    encryptCheckBox->setChecked(checked);
    encryptCheckBox->blockSignals(false);

    // Save to contact
    Contact contact = contactManager->getContact(peerOnion);
    contact.encryptionEnabled = checked;
    contactManager->updateContact(contact);
    qDebug() << "Encryption toggled for" << peerOnion << "to" << checked;
}



void MainWindow::exportMyInfo()
{
    // Create dialog
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

    // Pre-fill onion address and public key if available
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
            //publicKeyInput->setPlaceholderText(tr("**No public key present**\n"
              //                                    "**You can generate a keypair at a later time**\n"));
            // Proceed with export logic, but mark that no key is present
        } else {
            // User chose not to proceed; abort export
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

        // Prepare JSON object
        QJsonObject obj;
        obj["name"] = name;
        obj["onion"] = onion;
        if (!pubKey.isEmpty())
            obj["public_key"] = pubKey;
        if (!comments.isEmpty())
            obj["comments"] = comments;

        // Prepare file path
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
    // Check active connections
    if (!networkManager->getConnectedPeers().isEmpty()) {
        // 2. Show warning
        QMessageBox::StandardButton reply = QMessageBox::question(
            this,
            tr("Active Connections"),
            tr("You have open connections. This may disconnect you. Continue?"),
            QMessageBox::Yes | QMessageBox::No
            );

        // 3. Only update if user confirms
        if (reply == QMessageBox::No) return;
    }

    // proceed with update (if no connections or user confirmed)
    updateContactsList();

}

// new tor ports
void MainWindow::editTorrcPortsDialog()
{
    QString torrcPath = getTorrcDirPath();
    QFile file(torrcPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "Error", "Cannot open torrc file.");
        return;
    }

    // Read current ports
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

    // Current values (read-only)
    QLineEdit *currentChatPort = new QLineEdit(QString::number(port1));
    QLineEdit *currentHttpPort = new QLineEdit(QString::number(port2));
    QLineEdit *currentControlPort = new QLineEdit(QString::number(controlPort));
    QLineEdit *currentProxyPort = new QLineEdit(QString::number(proxyPort));

    currentChatPort->setReadOnly(true);
    currentHttpPort->setReadOnly(true);
    currentControlPort->setReadOnly(true);
    currentProxyPort->setReadOnly(true);

    // Editable fields for new values
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
    // Restore Defaults button
    QPushButton *restoreDefaultsBtn = new QPushButton("Restore Defaults", &dialog);
    layout->addWidget(restoreDefaultsBtn);
    QObject::connect(restoreDefaultsBtn, &QPushButton::clicked, [&]() {
        editChatPort->setText(defaultChatPort);
        editHttpPort->setText(defaultHttpPort);
        editControlPort->setText(defaultControlPort);
        editproxyPort->setText(defaultProxyPort);

    });

    // OK/Cancel buttons
    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel, &dialog);
    layout->addWidget(buttonBox);

    QObject::connect(buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    QObject::connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    if (dialog.exec() != QDialog::Accepted)
        return;


    // Validate and update
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

    // Read all lines
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

    // Write back
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

//drag and drop

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

// Override dropEvent to handle dropped .contact files
void MainWindow::dropEvent(QDropEvent *event)
{
    if (event->mimeData()->hasUrls()) {
        const QList<QUrl> urls = event->mimeData()->urls();
        for (const QUrl &url : urls) {
            if (url.isLocalFile() && url.toLocalFile().endsWith(".contact", Qt::CaseInsensitive)) {
                QString filePath = url.toLocalFile();

                // Read and parse the .contact file
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

                // Validate required fields
                if (!obj.contains("onion") || !obj.contains("name")) {
                    QMessageBox::warning(this, tr("Import Failed"), tr("Contact file missing required fields: %1").arg(filePath));
                    continue;
                }

                // Call addContact with both onion and the QJsonObject to prefill all fields
                addContact(obj.value("onion").toString(), obj);
            }
        }
        event->acceptProposedAction();
    } else {
        event->ignore();
    }
}

//event filter

bool MainWindow::eventFilter(QObject* obj, QEvent* event)
{
    qDebug() << "eventFilter called for" << obj << "event type" << event->type();


    if (event->type() == QEvent::Wheel) {
        QWheelEvent* wheelEvent = static_cast<QWheelEvent*>(event);
        qDebug() << "Modifiers:" << QApplication::keyboardModifiers();

        if (QApplication::keyboardModifiers() & Qt::ControlModifier) {
            qDebug() << "Ctrl+Wheel detected!";

            // Adjust font size
            if (wheelEvent->angleDelta().y() > 0)
                m_bubbleFontSize = std::min(m_bubbleFontSize + 1, 32);
            else if (wheelEvent->angleDelta().y() < 0)
                m_bubbleFontSize = std::max(m_bubbleFontSize - 1, 8);

            // Find the scroll area this event is for (optional: you can use the current tab)
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

// add a splitter between chat area and dock

void MainWindow::setupMainLayoutWithSplitter() {
    // Create splitter
    QSplitter* splitter = new QSplitter(this);
    splitter->setOrientation(Qt::Horizontal);

    // Setup dock widgets (contactsDock)
    setupDockWidgets(); // This creates contactsDock and dockContent

    // Setup chat area
    setupChatArea(); // This creates centralWidget and its layout

    // Add dock and chat area to splitter
    splitter->addWidget(contactsDock);      // Left side: contacts
    splitter->addWidget(centralWidget);     // Right side: chat area

    // Optionally set initial sizes
    splitter->setStretchFactor(0, 0); // contactsDock
    splitter->setStretchFactor(1, 1); // chat area
    splitter->setSizes({300, 800});   // Initial sizes at startup


    // Set splitter as the central widget
    setCentralWidget(splitter);
}


void MainWindow::checkFirstRun() {

    //QSettings settings;

    bool firstRun = settings.value("firstRun", true).toBool();

    if (firstRun) {
        qDebug() << "First run detected!";
        // Do your first-time setup here...

        settings.setValue("firstRun", false);

        QMessageBox::information(this, "Setup Complete",
                                 "To finalize setup, please restart the application after the UI is fully loaded");
        // Optionally: qApp->quit();
    } else {
        qDebug() << "Not first run.";
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

    // Delete config dir (can contain your settings)
    QDir dirConfig(configDir);
    qDebug() << configDir;
    bool deletedConfig = dirConfig.exists() ? dirConfig.removeRecursively() : true;
    qDebug() << "config dir is " << configDir;

    QFile configFile(getConfigDirPath() + ".conf");
    if (configFile.exists()) {
        if (!configFile.remove()) {
            qDebug() << "Failed to delete config file:" << configFile.fileName() << configFile.errorString();
        } else {
            qDebug() << "Config file deleted:" << configFile.fileName();
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

    // Populate font sizes from 8 to 16
    for (int i = 8; i <= 18; ++i) {
        fontSizeCombo->addItem(QString::number(i), i);
    }

    // Load font size from settings or use default 11
    int savedFontSize = settings.value("chatFontSize", 11).toInt();
    int fontSizeIndex = fontSizeCombo->findData(savedFontSize);
    if (fontSizeIndex != -1) {
        fontSizeCombo->setCurrentIndex(fontSizeIndex);
    } else {
        fontSizeCombo->setCurrentIndex(fontSizeCombo->findData(11));
    }
    m_bubbleFontSize = fontSizeCombo->currentData().toInt();

    // Connect combo box to update font size and save to settings
    connect(fontSizeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int idx) {
                int selectedFontSize = this->fontSizeCombo->itemData(idx).toInt();
                this->m_bubbleFontSize = selectedFontSize;
                this->messageInput->setFontPointSize(selectedFontSize);
                // Save to settings
                QSettings settings;
                settings.setValue("chatFontSize", selectedFontSize);

                // Update all message bubbles
                this->updateBubbleFontSizes();
            });


    toolbar->addWidget(fontSizeCombo);
}



