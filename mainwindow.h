#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QLineEdit>
#include <QPushButton>
#include <QTextEdit>
#include <QTextBrowser>
#include <QLabel>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QDockWidget>
#include <QDialog>
#include <QFormLayout>
#include <QDialogButtonBox>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>

#include "torconfig.h"
#include "networkmanager.h"
#include "contactlistwidget.h"
#include "chatmanager.h"
#include"contactmanager.h"
#include<QScrollArea>
#include<QToolBar>
#include <QSet>
#include <QPointer>
#include"emojipickerwidget.h"
#include"simplehttpfileserver.h"
#include<QProgressDialog>
#include<QSoundEffect>
#include"constants.h"
#include<QMenuBar>
#include"crypto.h"
#include<QCheckBox>
#include<QComboBox>
#include<QDragEnterEvent>
#include<QDropEvent>
#include"logviewerdialog.h"
#include<QEvent>
#include"securitymanager.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    //drag and drop
    bool eventFilter(QObject* obj, QEvent* event) override;


protected:
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;
private slots:
    void sendMessage();
    void updateStatus(const QString& status);
    void onPeerIdentified(const QString& peerOnion);

public slots:
    void connectToPeer();
    void handleConnectionStatus(bool connected, const QString& peerAddress);

    void handleNetworkError(const QString& error);
    void onAddContactClicked();
    void updateContactsList();
    void onContactSelected(const QString& onionAddress);
    void connectToContact(const QString& onionAddress);
    void handleIncomingMessage(const QString& senderAddress, const QString& message);
    void handleUnknownContact(const QString& senderAddress);
    void handleMessageAdded(const QString& peerAddress, const ChatMessage& message);
    void handleTabChanged(int index);
    void updateContactInfo(const QString& onionAddress);
    void handleMessageReceived(const QString& senderOnion, const QString& message);
    void handlePeerConnected(const QString& peerAddress);
    void handlePeerDisconnected(const QString& peerAddress = QString());
private:
    Ui::MainWindow *ui;

    // UI components
    QWidget* centralWidget = nullptr;
    QTextBrowser* chatDisplay;
    QTextEdit* messageInput;
    QPushButton* sendButton;
    QPushButton* fileButton;
    QLabel* statusLabel = nullptr;
    QPushButton* connectButton = nullptr;
    QLineEdit* peerAddressInput = nullptr;

    // Dock widgets
    QDockWidget* contactsDock;
    ContactListWidget* contactListWidget;
    QLineEdit* searchContactsEdit;

    // Network components
    TorConfig torConfig;
    NetworkManager* networkManager;
    ChatManager* chatManager;

    // State variables
    QString onionAddress;
    QString connectedPeerAddress;
    bool isConnected = false;
    bool isTorMode = true;  // Default to Tor mode

    // Setup methods
    void setupDockWidgets();
    void setupChatArea();
    void setupChatManager();

signals:
    void PeerIdentified(const QString& peerOnion);
    void messageReceived(const QString& message, const QString& senderOnion);
    void unknownContactMessageReceived(const QString& senderAddress);
private:
    QLabel* contactNameLabel;
    QLabel* contactAddressLabel;
    QLabel* contactCommentsLabel;
    ContactManager* contactManager;

private:
    QTabWidget* chatTabWidget;
    //QMap<QString, QTextBrowser*> chatDisplays;
    QMap<QString, QScrollArea*> chatDisplays;

    // Add these methods:
    void createChatTab(const QString& onionAddress);
    void switchToTab(const QString& onionAddress);
    //QLabel* contactStatusDot;

private slots:
    void handleEditContact(const QString& onion);
    void handleBlockContact(const QString& onion);
    void handleDeleteContact(const QString& onion);
    void handleAppExit();
    void openChatTabForPeer(const QString& onionAddress);
    // send file
    void sendFile();
private:

    //void updateContactDisplay(const QString& onion);
    void addContact(const QString &sendersOnion = QString(), const QJsonObject &contact = QJsonObject());
    QSet<QString> m_addContactDialogsOpen;
    QString lastAddedContactFriendlyName = QString();
    QMap<QString, QWidget*> chatTabWidgets; // onionAddress -> tabWidget
    QPushButton* openChatButton ;
    //emojies
    QStringList getAllEmojiResources();
    QStringList emojiPaths;
    EmojiPickerWidget* emojiPicker = nullptr;
    QPushButton* emojiButton;
    QWidget* inputWidget = nullptr;
    QDialog *pickerDialog = nullptr;
    void setupEmojiPickerDialog();
    // Http server
    SimpleHttpFileServer * httpServer = nullptr;
    //progress dialog while connecting
    //QProgressDialog* progressDialog = nullptr;
    // Clean temp WWW directories
    void cleanTmpWWWDirs();
    // sound system
private:
    bool play = true;
    void playSound(SoundType type, bool playIt);
    QMap<SoundType, QSoundEffect*> m_soundEffects;
    QScrollArea* scrollArea = nullptr;
private slots:
    // clear history
    void clearAllHistory();
    void clearCurrentTabHistory();
private:
    void setupMenuBar();
    QMenuBar* menubar = nullptr;
    QToolBar* getToolBar();
    QToolBar* toolbar;

private:
    QAction* newCircuitAction;
    QAction* getHttpOnionAction;
    QAction* getWWWDirAction;
    QAction* startServerAction;
    QAction* stopServerAction;
    QAction* restartServerAction;
    QAction* clearCurrentHistoryAction;
    QAction* clearAllHistoryAction;
    QAction* newHttpOnionAction;
    QAction* newChatOnionAction;
    QAction* factoryResetAction;
    QAction* toggleMenuBarAction;
    QAction* copyPubKeyAction;
    QAction* createKeypairAction;
    QAction* exportMyInfoAction;
    QAction* copyOnionAction;
    QAction* toggleLoggingAction;
    QAction *editTorrcPortsAction ;
    QAction* logViewerAction;
    QAction* muteAction;
    QAction *actionTorBridge;

    void setupActions();
    QComboBox* rsaBitsCombo;
    QComboBox* statusCombo;

    //crypto
    Crypto crypt;
    QString encryptMessageForPeer(const QString& peerOnion, const QString& plainText);
    QString decryptMessageFromPeer(const QString& peerOnion, const QString& encryptedBase64);
    QCheckBox* encryptCheckBox;
private slots:
    void onEncryptToggled(bool checked);
    void onCreateNewKeypair();
    void exportMyInfo();
private:
    QWidget* dockContent = nullptr;
    QVBoxLayout* dockLayout = nullptr;
    void checkAndUpdateContacts();
    QString myOnion;
    // new tor ports
    void loadPortsFromTorrc();
//font size for msgbubble
    int m_bubbleFontSize = 11; // Default font size
//change contactselected whenchanging tab
    bool updatingSelection = false;
// add a splitter
    void setupMainLayoutWithSplitter();


private slots:
    void editTorrcPortsDialog();
    void performFactoryReset();

private:
    SecurityManager *securityManager = nullptr;
    void checkFirstRun();
    QSettings settings;
    QCheckBox* enableClearAllChatsCheckBox;
    QCheckBox* muteSoundsCheckBox;
    QCheckBox* enableLoggerCheckBox;
    void updateBubbleFontSizes();
    QComboBox *fontSizeCombo = nullptr;
    void addFontSizeCombo();
public:
};

#endif // MAINWINDOW_H

