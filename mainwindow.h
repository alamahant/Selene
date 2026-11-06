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
#include"httpclientdialog.h"
#include"bridgemanagerdialog.h"
#include"sitemanagerdialog.h"
#include"sitemanager.h"
#include"torprocess.h"
#include"torcirquitworker.h"
#include<QThread>
#include<QToolButton>
#include<QMenu>
#include"dropboxdialog.h"
#include "mailmanager.h"
#include "mailworker.h"
#include"maildialog.h"
#include<QThread>

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

    TorProcess *torProcess() const;

protected:
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

    QWidget* centralWidget = nullptr;
    QTextBrowser* chatDisplay;
    QTextEdit* messageInput;
    QPushButton* sendButton;
    QPushButton* fileButton;
    QLabel* statusLabel = nullptr;
    QPushButton* connectButton = nullptr;
    QLineEdit* peerAddressInput = nullptr;

    QDockWidget* contactsDock;
    ContactListWidget* contactListWidget;
    QLineEdit* searchContactsEdit;

    TorConfig torConfig;
    NetworkManager* networkManager;
    ChatManager* chatManager;

    QString onionAddress;
    QString connectedPeerAddress;
    bool isConnected = false;
    bool isTorMode = true;  // Default to Tor mode

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
    QMap<QString, QScrollArea*> chatDisplays;

    void createChatTab(const QString& onionAddress);
    void switchToTab(const QString& onionAddress);

private slots:
    void handleEditContact(const QString& onion);
    void handleBlockContact(const QString& onion);
    void handleDeleteContact(const QString& onion);
    void handleAppExit();
    void openChatTabForPeer(const QString& onionAddress);
    void sendFile();
private:

    void addContact(const QString &sendersOnion = QString(), const QJsonObject &contact = QJsonObject());
    QSet<QString> m_addContactDialogsOpen;
    QString lastAddedContactFriendlyName = QString();
    QMap<QString, QWidget*> chatTabWidgets; // onionAddress -> tabWidget
    QPushButton* openChatButton ;
    QStringList getAllEmojiResources();
    QStringList emojiPaths;
    EmojiPickerWidget* emojiPicker = nullptr;
    QPushButton* emojiButton;
    QWidget* inputWidget = nullptr;
    QDialog *pickerDialog = nullptr;
    void setupEmojiPickerDialog();
    SimpleHttpFileServer * httpServer = nullptr;
    void cleanTmpWWWDirs();
private:
    bool play = true;
    void playSound(SoundType type, bool playIt);
    QMap<SoundType, QSoundEffect*> m_soundEffects;
    QScrollArea* scrollArea = nullptr;
private slots:
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
    QAction *getContactsDirPathAction;
    QAction *openHttpClientAction;
    QAction *openBridgeDialogAction;

    void setupActions();
    QComboBox* rsaBitsCombo;
    QComboBox* statusCombo;

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
    void loadPortsFromTorrc();
    int m_bubbleFontSize = 11; // Default font size
    bool updatingSelection = false;
    void setupMainLayoutWithSplitter();


private slots:
    void editTorrcPortsDialog();
    void performFactoryReset();
    void createSymlink();
    void openFolder();
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
    HttpClientDialog *httpClientDialog = nullptr;
    QString encryptFileForPeer(const QString &filePath, const QString &peerOnion);
    BridgeManagerDialog *bridgeDialog = nullptr;
    QLabel* contactGroupLabel;
    void setupCornerWidget();
    SiteManagerDialog* m_siteDialog = nullptr;
    SiteManager* m_siteManager = nullptr;
    QAction* openSiteManagerAction;
    TorProcess* m_torProcess = nullptr;
    QProgressBar* m_bootstrapBar = nullptr;

private slots:
    void onBootstrapProgress(int percent);
    void onTorReady();
    void onCircuitCountries(const QStringList& countries);

private:
    TorCircuitWorker* m_circuitWorker = nullptr;
    QThread* m_circuitThread = nullptr;
    QToolButton* m_circuitButton = nullptr;
    QMenu* m_circuitMenu = nullptr;
    QStringList m_circuitCountries;
    QTimer* tick =nullptr;
    QString flagFor(const QString& cc);
private:
    QString m_dropboxUuid;
    bool m_dropboxPublished = false;
    DropboxDialog* m_dropboxDialog = nullptr;

    void showDropboxDialog();
    void onNewDropboxUuid(bool removeOld);
    void onPublishDropbox();
    void onUnpublishDropbox();
    void updateDropboxDialog();

    QString createDropboxPage();
    QString dropboxUrl(const QString& uuid) const;
    QAction* m_dropboxAction;
    void saveDropboxUuid();
    void loadDropboxUuid();

private:
    MailManager* m_mailManager = nullptr;
    MailWorker*  m_mailWorker  = nullptr;
    QThread*     m_mailThread  = nullptr;
    QAction* getMailOnionAction = nullptr;
    QAction* newMailOnionAction = nullptr;
    MailDialog* m_mailDialog = nullptr;
private slots:
    void openMailDialog();
    void onMailReceived(int count);
};

#endif // MAINWINDOW_H

