#include"helpmenudialog.h"
#include <QApplication>
#include"constants.h"
#include<QString>

HelpMenuDialog::HelpMenuDialog(HelpType type, QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(getTitle(type));
    setModal(true);
    resize(600, 500);

    setupUI();
    loadContent(type);
}

void HelpMenuDialog::setupUI()
{
    m_layout = new QVBoxLayout(this);

    m_contentArea = new QTextEdit(this);
    m_contentArea->setReadOnly(true);

    m_closeButton = new QPushButton("Close", this);
    connect(m_closeButton, &QPushButton::clicked, this, &QDialog::accept);

    m_layout->addWidget(m_contentArea);
    m_layout->addWidget(m_closeButton);
}

void HelpMenuDialog::loadContent(HelpType type)
{
    QString content;

    switch (type) {
    case HelpType::About:
        content = getAboutContent();
        break;
    case HelpType::Features:
        content = getFeaturesContent();
        break;
    case HelpType::Instructions:
        content = getInstructionsContent();
        break;
    case HelpType::BestPractices:
        content = getBestPracticesContent();
        break;
    case HelpType::WhatsNew:
        content = getWhatsNewContent();
        break;
    case HelpType::Shortcuts:
        content = getShortcutsContent();
        break;
    case HelpType::Security:
        content = getSecurityContent();
        break;
    case HelpType::TwoFA:
        content = getTwoFAContent();
        break;
    case HelpType::DataManagement:
        content = getDataManagementContent();
        break;
    case HelpType::DownloadManagement:
        content = getDownloadManagerContent();
        break;
    case HelpType::onSitesAndSessions:
        content = getSitesSessionsContent();
        break;
    case HelpType::onSecurity:
        content = getOnSecurityContent();
        break;
    case HelpType::onNewStorageSystem:
        content = getOnNewStorageSystemContent();
        break;
    case HelpType::onNamedProfiles:
        content = getOnNamedProfilesContent();
        break;
    }

    m_contentArea->setHtml(content);
}

QString HelpMenuDialog::getTitle(HelpType type)
{
    switch (type) {
    case HelpType::About: return "About Selene";
    case HelpType::Features: return "Features";
    case HelpType::Instructions: return "Instructions";
    case HelpType::BestPractices: return "Best Practices";
    case HelpType::WhatsNew: return "What's New";
    case HelpType::Shortcuts: return "Keyboard Shortcuts";
    case HelpType::Security: return "Selene Security";
    case HelpType::TwoFA: return "Selene 2FA Utility";
    case HelpType::DataManagement: return "Selene Data Management";
    case HelpType::DownloadManagement: return "Selene Download Management";
    case HelpType::onSitesAndSessions: return "On Sites And Sessions";
    case HelpType::onSecurity: return "On Security";
    case HelpType::onNewStorageSystem: return "On the New Storage System";
    case HelpType::onNamedProfiles: return "On Shared Named Profiles";

    default: return "Help";
    }
}

QString HelpMenuDialog::getBestPracticesContent()
{
    return QString();  // TODO: Implement
}

QString HelpMenuDialog::getWhatsNewContent()
{
    return QString();  // TODO: Implement
}

QString HelpMenuDialog::getShortcutsContent()
{
    return QString();  // TODO: Implement
}

QString HelpMenuDialog::getAboutContent() {
    return QString(R"(
<div style="text-align: center; font-family: Arial, sans-serif;">
<h1 style="color: #2c3e50; margin-bottom: 10px;">🌸 Selene</h1>
<h3 style="color: #7f8c8d; margin-bottom: 20px;">Tor-based P2P Chat and File Sharing Manager</h3>
<p style="font-size: 16px; margin-bottom: 20px;">
Selene is a comprehensive peer-to-peer chat and file sharing application built on the Tor network.
It enables secure, private communication and seamless file transfers between users, leveraging the anonymity and resilience of Tor.
</p>
<div style="background-color: #ecf0f1; padding: 15px; border-radius: 8px; margin: 20px 0;">
<p style="margin: 5px 0;"><strong>Version:</strong> %1</p>
<p style="margin: 5px 0;"><strong>Built with:</strong> Qt Framework</p>
<p style="margin: 5px 0;"><strong>Platform:</strong> Cross-platform</p>
</div>
<div style="margin: 30px 0;">
<h4 style="color: #2c3e50;">Key Features</h4>
<p style="text-align: left; margin: 10px 20px;">
• True peer-to-peer messaging: messages travel exclusively over the Tor network, never leaving Tor at any point<br>
• End-to-end RSA encryption for messages, with selectable key lengths: 2048, 4096, or 8192 bits<br>
• File sharing via ephemeral Tor hidden service HTTP servers: recipients access files directly using the provided onion address in Tor Browser or any Tor-enabled private window<br>
</p>
</div>
<hr style="margin: 30px 0; border: 1px solid #bdc3c7;">
<div style="margin: 20px 0;">
<p style="margin: 5px 0; color: #7f8c8d;">
<strong>Copyright © 2025 Alamahant</strong>
</p>
<p style="margin: 5px 0; font-size: 12px; color: #95a5a6;">
All rights reserved. This software is provided as-is without warranty.
</p>
</div>
<div style="margin: 20px 0;">
<p style="font-size: 14px; color: #7f8c8d;">
Made with ❤️ for privacy advocates and secure communication enthusiasts
</p>
</div>
</div>
)").arg(APP_VERSION);
}

QString HelpMenuDialog::getFeaturesContent()
{
    return R"(
<div style="text-align: center; font-family: Arial, sans-serif;">

<h1 style="color: #2c3e50; margin-bottom: 10px;">🌸 Selene Features</h1>
<h3 style="color: #7f8c8d; margin-bottom: 20px;">Advanced Tor-based P2P Communication & File Sharing</h3>

<div style="background-color: #ecf0f1; padding: 15px; border-radius: 8px; margin: 20px 0;">

<h4 style="color: #2c3e50;">Feature Highlights</h4>
<ul style="text-align: left; margin: 10px 40px; font-size: 15px; background: transparent; list-style-position: inside;">
<li style="background: transparent;"><strong>Request New Tor Circuit:</strong> Instantly request a new Tor circuit to improve connectivity and reduce delays, ensuring reliable communication even in challenging network conditions.</li>
<li style="background: transparent;"><strong>Configurable Tor Ports:</strong> Flexibly edit Tor ports for chat, HTTP, control, and proxy services. This is especially useful when running multiple Tor daemons or other Tor-based applications in parallel.</li>
<li style="background: transparent;"><strong>Onion Service Management:</strong> Easily request new onion addresses for both chat and HTTP services, enhancing privacy and operational flexibility.</li>
<li style="background: transparent;"><strong>Customizable Chat Status & Font Size:</strong> Set your chat status as 'Available' or 'Away' and adjust chat font size via intuitive combo boxes for a personalized experience.</li>
<li style="background: transparent;"><strong>Contact Export & Import:</strong> Export your contact information to a .contact file for easy sharing. Recipients can simply drag and drop the file into the UI to add contacts, with manual addition also supported.</li>
<li style="background: transparent;"><strong>Tor-native End-to-End Encryption:</strong> All communication remains strictly within the Tor network, leveraging Tor's built-in encryption for maximum privacy. Additionally, users may enable extra RSA encryption (2048–8192 bits) for messages, providing an extra layer of security.</li>
<li style="background: transparent;"><strong>Advanced Logging System:</strong> Comprehensive logging captures extensive data for diagnostics and auditing. Includes a powerful log viewer with search and truncation capabilities, plus options to enable/disable logging for optimal performance.</li>
<li style="background: transparent;"><strong>Rich Messaging & File Sharing:</strong> Send plain or encrypted messages, share emojis via a dedicated emoji class, and transfer files seamlessly. Files placed in <code>~/Documents/Selene/www</code> can be shared via an ephemeral onion HTTP server, with the onion URL easily accessible from the toolbar. Direct file sharing in chat is supported via file dialog, with automatic secure link generation for recipients.</li>
<li style="background: transparent;"><strong>Per-Peer Encryption Control:</strong> Enable or disable additional encryption for each peer using a simple checkbox, tailoring security to your needs.</li>
<li style="background: transparent;"><strong>Factory Reset:</strong> Restore the app to its original state with a single action. All data is wiped except for contacts and shared files stored in <code>~/Documents/Selene</code>.</li>
<li style="background: transparent;"><strong>Quick Access Toolbar:</strong> Convenient toolbar buttons allow instant copying of your chat onion, HTTP onion, and shared directory paths to the clipboard.</li>
<li style="background: transparent;"><strong>Chat History Management:</strong> Easily clear chat messages and history per peer or for all peers. Optionally enable automatic history cleaning at app startup for enhanced privacy.</li>
<li style="background: transparent;">
<strong>Password Protection at Startup:</strong>
Enhance your privacy with optional password protection. When enabled, Selene requires a master password at startup to access your data. The system supports secure password creation, change, and validation, with robust SHA-256 hashing and salting. After multiple failed attempts, users can choose to exit or perform a factory reset. Password protection can be enabled or disabled at any time for flexible security management.
</li>
<li style="background: transparent;">
<strong>Modern Contact Cards UI:</strong>
Contacts are displayed as modern cards, each showing the contact's name and status (<span style='color:#27ae60;'>Connected</span>, <span style='color:#e67e22;'>Disconnected</span>, <span style='color:#7f8c8d;'>Offline</span>). Each card includes <b>Connect</b>, <b>Edit</b>, <b>Delete</b>, and <b>Block</b> buttons for intuitive management.
</li>
<li style="background: transparent;">
  <strong>Export & Share Your Contact:</strong>
  Easily export your own contact details as a <code>.contact</code> file. You can send this file by email or any other method to your peers. They can simply drag and drop the file from their file manager into the Selene UI to instantly add you as a contact. Manual contact addition is also fully supported for maximum flexibility.
</li>
<li style="background: transparent;">
<strong>Advanced Contact Search:</strong>
Find contacts instantly with a powerful search bar:
<ul style="margin: 5px 0 5px 20px; font-size: 14px;">
  <li><b>By name:</b> Normal pattern search (case-insensitive).</li>
  <li><b>By onion address:</b> Start with <code>#o</code> (e.g., <code>#oabc123</code>).</li>
  <li><b>By blocked contacts:</b> Use <code>#b</code> to show only blocked contacts.</li>
  <li><b>By comments:</b> Use <code>#c</code> followed by text to search within comments.</li>
</ul>
</li>
</ul>

</div>

<div style="margin: 30px 0;">
<p style="font-size: 14px; color: #7f8c8d;">
Selene is designed for privacy advocates and secure communication enthusiasts, offering a robust suite of features for confidential, flexible, and user-friendly Tor-based messaging and file sharing.
</p>
</div>

</div>
)";
}

QString HelpMenuDialog::getInstructionsContent()
{
    return R"(
<div style="font-family: Arial, sans-serif; line-height: 1.6; color: #2c3e50; background: transparent;">

<h1 style="color: #e74c3c; text-align: center; margin-bottom: 20px; background: transparent;">📋 How to Use Selene</h1>

<div style="background: transparent; padding: 15px; border-radius: 8px; margin-bottom: 25px;">
<h2 style="color: #2c3e50; background: transparent;">1. Exporting & Sharing Your Contact</h2>
<ul style="margin: 10px 0 10px 25px;">
<li>
  <strong>Create an RSA key pair</strong> for message encryption (optional but recommended). This key can be included in your <code>.contact</code> file for secure communication.
</li>
<li>
  In the Chat area toolbar, press <strong>Export Contact Information</strong> to save your details as a <code>.contact</code> file.
</li>
<li>
  <strong>Deliver your <code>.contact</code> file</strong> to peers via email or any preferred method.
</li>
<li>
  Your peers can <strong>drag and drop</strong> the file from their file manager into the Selene UI to instantly add you as a contact.
</li>
<li>
  Manual contact addition is also available for maximum flexibility.
</li>
</ul>
</div>

<div style="background: transparent; padding: 15px; border-radius: 8px; margin-bottom: 25px;">
<h2 style="color: #3498db; background: transparent;">2. Connecting & Chatting with Peers</h2>
<ul style="margin: 10px 0 10px 25px;">
<li>
  <strong>Select a contact card</strong> and press <strong>Connect</strong>. If your peer is online, you will be connected and can start chatting.
</li>
<li>
  <strong>Connection times may vary:</strong> Be patient—connecting can take anywhere from 2 to 20 seconds depending on Tor network conditions.
</li>
<li>
  <strong>TIP:</strong> If connection fails, go to <strong>Toolbar → Tor Area → Request New Tor Circuit</strong>. This usually resolves slow connections. After requesting a new circuit, press Connect again.
</li>
<li>
  You can connect to as many peers as you like, just like any other chat app.
</li>
<li>
  If you get disconnected while chatting (rare, but possible due to Tor's nature), simply reconnect. Tor circuits may change or drop, causing temporary disconnections.
</li>
<li>
  <em>Tor is designed for privacy and anonymity, but its routing can occasionally cause delays or disconnects. Reconnecting is quick and easy!</em>
</li>
</ul>
</div>

<div style="background: transparent; padding: 15px; border-radius: 8px; margin-bottom: 25px;">
<h2 style="color: #f39c12; background: transparent;">3. Searching Contacts</h2>
<ul style="margin: 10px 0 10px 25px;">
<li style="background: transparent;">
<strong>Advanced Contact Search:</strong>
Find contacts instantly with a powerful search bar:
<ul style="margin: 5px 0 5px 20px; font-size: 14px;">
<li><b>By name:</b> Normal pattern search (case-insensitive).</li>
<li><b>By onion address:</b> Start with <code>#o</code> (e.g., <code>#oabc123</code>).</li>
<li><b>By blocked contacts:</b> Use <code>#b</code> to show only blocked contacts.</li>
<li><b>By comments:</b> Use <code>#c</code> followed by text to search within comments.</li>
</ul>
</li>
</ul>
</div>


<div style="background: transparent; padding: 15px; border-radius: 8px; margin-bottom: 25px;">
<h2 style="color: #27ae60; background: transparent;">4. Sharing Files via Tor HTTP Server</h2>
<ul style="margin: 10px 0 10px 25px;">
<li>
  Place any files you wish to share in the <code>~/Documents/Selene/www</code> directory.
</li>
<li>
  In the Server area of the toolbar, press <strong>Start HTTP Server</strong> to begin sharing.
</li>
<li>
  To find the exact document root directory, press <strong>Copy WWW Dir Path</strong> in the toolbar. This copies the path to your clipboard for easy navigation.
</li>
<li>
  After starting the server, get your HTTP onion address by clicking <strong>Copy HTTP Onion</strong> in the toolbar. Share this onion address with your peers.
</li>
<li>
  As long as Selene is running, your peers can access shared files via Tor Browser or any Tor-enabled browser window.
</li>
</ul>
</div>

<div style="background: transparent; padding: 15px; border-radius: 8px; margin-bottom: 25px;">
<h2 style="color: #6c3483; background: transparent;">5. Sending Files Privately in Chat</h2>
<ul style="margin: 10px 0 10px 25px;">
<li>
  While chatting with a peer, press the <strong>Send File</strong> button.
</li>
<li>
  Select files using the file dialog and confirm.
</li>
<li>
  In the chat window, both you and your peer will receive an onion URL containing a hidden, UUID-named directory.
</li>
<li>
  Your peer can access the file via Tor Browser or any Tor-enabled browser.
</li>
</ul>
</div>

<div style="background: transparent; padding: 15px; border-radius: 8px; margin-bottom: 25px;">
<h2 style="color: #6c3483; background: transparent;">6. Encryption & Key Management</h2>
<ul style="margin: 10px 0 10px 25px;">
<li>
  <strong>RSA keys (2048, 4096, or 8192 bits)</strong> are needed for message encryption.
</li>
<li>
  If both you and your peer have exchanged public keys (usually by importing each other's <code>.contact</code> files), you can enable encryption by ticking the checkbox next to the Send File button.
</li>
<li>
  <strong>NOTE:</strong> The maximum message length depends on the key size chosen when creating your key pair. Higher bit keys allow more characters per encrypted message:<br>
  2048 bits → just under 256 characters<br>
  4096 bits → just under 512 characters<br>
  8192 bits → just under 1024 characters
</li>
</ul>
</div>

<div style="background: transparent; padding: 15px; border-radius: 8px; margin-bottom: 25px;">
<h2 style="color: #e67e22; background: transparent;">7. Blocking & Unblocking Contacts</h2>
<ul style="margin: 10px 0 10px 25px;">
<li>
  Block any peer by pressing the <strong>Block</strong> button on their contact card. Blocked peers cannot connect with you.
</li>
<li>
  You can unblock contacts at any time.
</li>
</ul>
</div>

<div style="background: transparent; padding: 15px; border-radius: 8px; margin-bottom: 25px;">
<h2 style="color: #16a085; background: transparent;">8. Chat Status Management</h2>
<ul style="margin: 10px 0 10px 25px;">
<li>
  Use the <strong>Chat Status</strong> combo box in the toolbar to set your status to <strong>Available</strong> or <strong>Away</strong>.
</li>
<li>
  When set to <strong>Away</strong>, others cannot contact you, but you can still initiate chats.
</li>
</ul>
</div>

<div style="background: transparent; padding: 15px; border-radius: 8px; margin-bottom: 25px;">
<h2 style="color: #2c3e50; background: transparent;">Tips & Best Practices</h2>
<ul style="margin: 10px 0 10px 25px;">
<li>
  <strong>Drag & Drop:</strong> The easiest way to add contacts is by dragging their <code>.contact</code> file into the Selene UI.
</li>
<li>
  <strong>Manual Addition:</strong> You can always add contacts manually if you prefer.
</li>
<li>
  <strong>Tor Network:</strong> Connection speed and stability depend on Tor's routing. If you experience delays or disconnects, try requesting a new circuit.
</li>
<li>
  <strong>Multiple Connections:</strong> You can chat with multiple peers simultaneously—just like any modern chat app.
</li>
<li>
  <strong>Security:</strong> Always exchange public keys for encrypted messaging. Higher bit keys allow longer messages per encrypted block.
</li>
<li>
  <strong>Enable/Disable Logging:</strong> Selene implements a dedicated Logger class.You can however disable logging for blazing fast speed and responsiveness. You can safely do so when you have established optimal functioning.
</li>

<li>
  <strong>Sending Onion Urls:</strong> When sending onion urls to your peers -- like for example when sharing www directory path etc is easier to not mix them with plain text. This makes it easier for the recipient to copy and use the sent url more effectively.
</li>

<li>
  <strong>File Downloads:</strong> All shared files are sent with download-enforcing headers.
  If your browser opens them instead (common for PDFs/videos), then please
  <strong>right-click → "Save link as"</strong>.
</li>

</ul>
</div>

<div style="text-align: center; margin-top: 40px; padding: 15px; background: #f8f9fa; border-radius: 8px; border-left: 4px solid #3498db;">
<h3 style="color: #2c3e50; margin-top: 0;">Video Tutorial Available</h3>
<p style="margin-bottom: 5px;">For a visual guide to all these features, visit:</p>
<p style="margin: 0; font-family: monospace; word-break: break-all;">https://www.youtube.com/watch?v=zZBf6axINHw</p>
</div>


<div style="background: transparent; padding: 15px; border-radius: 8px; margin-bottom: 25px;">
<h2 style="color: #2c3e50; background: transparent;">Colohon: The Selene Philosophy</h2>
<ul style="margin: 10px 0 10px 25px;">
<li>
When we, the developers, started working on Selene, what we found most appealing about using Tor as a medium for a chat application is that it is truly <strong>peer-to-peer</strong>: no need for central servers, no need to open ports in routers or firewalls. This simplicity and directness was incredibly attractive—no STUN, no NAT traversal, no complicated setup.
</li>
<li>
However, we quickly realized that many users would find Tor’s <strong>privacy and security features</strong> even more compelling. So, we added message encryption to make Selene almost impenetrable.
</li>
<li>
Different people can approach and use Selene from different angles—and all of them are valid. Whether you value peer-to-peer simplicity, privacy, or security, Selene is here for you. Use it however you please, and have fun!
</li>
</ul>
</div>

<div style="text-align: center; margin: 30px 0; padding: 20px; background: transparent; border-radius: 8px;">
<h3 style="color: #495057; margin-top: 0; background: transparent;">Selene: Secure, Private, and Flexible Tor-based Communication</h3>
<p style="margin-bottom: 0; background: transparent;">Enjoy confidential messaging, file sharing, and advanced contact management—all powered by Tor.</p>
</div>

</div>
)";
}





QString HelpMenuDialog::getSecurityContent()
{
    return R"(
        <div style="font-family: Arial, sans-serif; line-height: 1.6; color: #2c3e50;">
            <h1 style="color: #e74c3c; text-align: center; margin-bottom: 20px;">🔐 Security Features</h1>

            <div style="background-color: #ecf0f1; padding: 15px; border-radius: 8px; margin-bottom: 25px;">
                <h2 style="color: #2c3e50; margin-top: 0;">Master Password Protection</h2>
                <p>Selene includes a comprehensive security system to protect your saved websites, sessions, and sensitive data.</p>
            </div>

            <h2 style="color: #3498db; border-bottom: 2px solid #3498db; padding-bottom: 5px;">🛡️ Password Protection Features</h2>

            <div style="margin: 20px 0;">
                <ul style="margin: 10px 0; padding-left: 20px;">
                    <li>Master password requirement on startup</li>
                    <li>Secure password hashing with salt encryption</li>
                    <li>Failed attempt protection (5 attempts maximum)</li>
                    <li>Factory reset option for forgotten passwords</li>
                </ul>
            </div>

            <h2 style="color: #27ae60; border-bottom: 2px solid #27ae60; padding-bottom: 5px;">🔧 How to Enable Password Protection</h2>

            <div style="margin: 20px 0;">
                <ol style="margin: 10px 0; padding-left: 25px;">
                    <li>Go to Security menu → "Require Password on Startup"</li>
                    <li>Read the security notice and click OK</li>
                    <li>Enter your new master password</li>
                    <li>Confirm your password</li>
                    <li>Selene will now require this password on every startup</li>
                </ol>
            </div>

            <div style="background-color: #fff3cd; padding: 15px; border-radius: 8px; margin: 15px 0;">
                <h3 style="color: #856404; margin-top: 0;">⚠️ Important Security Notes</h3>
                <ul style="margin: 10px 0; padding-left: 20px;">
                    <li>Choose a strong, memorable password</li>
                    <li>Write it down in a safe place</li>
                    <li>If you forget it, you'll need to factory reset</li>
                    <li>Password is encrypted and stored securely</li>
                </ul>
            </div>

            <h2 style="color: #f39c12; border-bottom: 2px solid #f39c12; padding-bottom: 5px;">🔄 Changing Your Master Password</h2>

            <div style="margin: 20px 0;">
                <ol style="margin: 10px 0; padding-left: 25px;">
                    <li>Go to Security menu → "Change Master Password"</li>
                    <li>Enter your new password</li>
                    <li>Confirm the new password</li>
                    <li>Password is updated immediately</li>
                </ol>
            </div>

            <h2 style="color: #e74c3c; border-bottom: 2px solid #e74c3c; padding-bottom: 5px;">🚨 Failed Login Protection</h2>

            <div style="margin: 20px 0;">
                <p><strong>Maximum 5 password attempts allowed</strong></p>
                <p>After 5 failed attempts, you get two options:</p>
                <ul style="margin: 10px 0; padding-left: 20px;">
                    <li><strong>Exit Application</strong></li>
                    <li><strong>Factory Reset</strong> (clears all data and security settings)</li>
                </ul>
            </div>

            <h2 style="color: #9b59b6; border-bottom: 2px solid #9b59b6; padding-bottom: 5px;">🔄 Factory Reset</h2>

            <div style="margin: 20px 0;">
                <p>If you forget your master password, factory reset will:</p>
                <ul style="margin: 10px 0; padding-left: 20px;">
                    <li>Remove all security settings</li>
                    <li>Clear the master password</li>
                    <li>Reset password protection to disabled</li>
                    <li>Allow you to start fresh</li>
                </ul>
            </div>

            <h2 style="color: #1abc9c; border-bottom: 2px solid #1abc9c; padding-bottom: 5px;">⚙️ Security Menu Options</h2>

            <div style="margin: 20px 0;">
                <ul style="margin: 10px 0; padding-left: 20px;">
                    <li><strong>"Require Password on Startup"</strong> - Toggle password protection on/off</li>
                    <li><strong>"Change Master Password"</strong> - Update your existing password</li>
                </ul>
            </div>

            <h2 style="color: #e67e22; border-bottom: 2px solid #e67e22; padding-bottom: 5px;">🔒 Protection States</h2>

            <div style="margin: 20px 0;">
                <h3 style="color: #d35400;">When Password Protection is Active:</h3>
                <ul style="margin: 10px 0; padding-left: 20px;">
                    <li>Selene prompts for password on every startup</li>
                    <li>All your websites, sessions, and data remain encrypted</li>
                    <li>No access to application features without correct password</li>
                </ul>

                <h3 style="color: #d35400;">When Password Protection is Disabled:</h3>
                <ul style="margin: 10px 0; padding-left: 20px;">
                    <li>Selene starts immediately without password prompt</li>
                    <li>All features accessible without authentication</li>
                    <li>Data remains saved but unprotected</li>
                </ul>
            </div>

            <h2 style="color: #16a085; border-bottom: 2px solid #16a085; padding-bottom: 5px;">💡 Best Practices</h2>

            <div style="margin: 20px 0;">
                <ul style="margin: 10px 0; padding-left: 20px;">
                    <li>Use a unique password not used elsewhere</li>
                    <li>Include numbers, letters, and special characters</li>
                    <li>Avoid easily guessable information</li>
                    <li>Keep a secure backup of your password</li>
                    <li>Enable password protection if you store sensitive login information</li>
                </ul>
            </div>

            <h2 style="color: #8e44ad; border-bottom: 2px solid #8e44ad; padding-bottom: 5px;">🔐 Security Implementation</h2>

            <div style="margin: 20px 0;">
                <ul style="margin: 10px 0; padding-left: 20px;">
                    <li>SHA-256 encryption with custom salt</li>
                    <li>No plain text password storage</li>
                    <li>Secure settings storage</li>
                    <li>Memory-safe password handling</li>
                </ul>
            </div>

            <div style="background-color: #d1ecf1; padding: 15px; border-radius: 8px; margin: 25px 0;">
                <h3 style="color: #0c5460; margin-top: 0;">🛡️ Complete Protection</h3>
                <p style="margin-bottom: 0;">This security system ensures your browsing profiles, saved websites, sessions, and any stored login references remain protected even if someone gains access to your computer.</p>
            </div>
        </div>
    )";
}

QString HelpMenuDialog::getTwoFAContent()
{
    return R"(
        <div style="font-family: Arial, sans-serif; line-height: 1.6; color: #2c3e50;">
            <h1 style="color: #e74c3c; text-align: center; margin-bottom: 20px;">🔐 2FA Code Generator</h1>

            <div style="background-color: #ecf0f1; padding: 15px; border-radius: 8px; margin-bottom: 25px;">
                <h2 style="color: #2c3e50; margin-top: 0;">Two-Factor Authentication Manager</h2>
                <p>Selene includes a built-in Two-Factor Authentication (2FA) code generator that helps you manage and generate time-based one-time passwords (TOTP) for your accounts.</p>
            </div>

            <h2 style="color: #3498db; border-bottom: 2px solid #3498db; padding-bottom: 5px;">❓ What is 2FA?</h2>

            <div style="margin: 20px 0;">
                <p>Two-Factor Authentication adds an extra layer of security to your accounts by requiring a second form of verification beyond just your password. This usually involves a 6-digit code that changes every 30 seconds.</p>
            </div>

            <h2 style="color: #27ae60; border-bottom: 2px solid #27ae60; padding-bottom: 5px;">🚀 Key Features</h2>

            <div style="margin: 20px 0;">
                <ul style="margin: 10px 0; padding-left: 20px;">
                    <li>Generate 6-digit TOTP codes for any 2FA-enabled account</li>
                    <li>Real-time code updates every 30 seconds</li>
                    <li>Visual countdown timer showing when codes refresh</li>
                    <li>One-click code copying to clipboard</li>
                    <li>Secure local storage of account secrets</li>
                    <li>Support for multiple accounts from different services</li>
                </ul>
            </div>

            <h2 style="color: #f39c12; border-bottom: 2px solid #f39c12; padding-bottom: 5px;">📱 How to Access</h2>

            <div style="margin: 20px 0;">
                <ul style="margin: 10px 0; padding-left: 20px;">
                    <li>Open the 2FA Manager from the Toolbar icon or the Tools Menu</li>
                    <li>The manager opens in a separate window</li>
                    <li>Resizable interface with accounts list and code display</li>
                </ul>
            </div>

            <h2 style="color: #9b59b6; border-bottom: 2px solid #9b59b6; padding-bottom: 5px;">➕ Adding 2FA Accounts</h2>

            <div style="margin: 20px 0;">
                <ol style="margin: 10px 0; padding-left: 25px;">
                    <li>Click "Add Account" button</li>
                    <li>Enter account name (e.g., "GitHub", "Google", "Discord")</li>
                    <li>Paste the secret key from the website's 2FA setup</li>
                    <li>Optionally enter the issuer/company name</li>
                    <li>Click OK to save</li>
                </ol>
            </div>

            <div style="background-color: #fff3cd; padding: 15px; border-radius: 8px; margin: 15px 0;">
                <h3 style="color: #856404; margin-top: 0;">🔍 Where to Find Secret Keys</h3>
                <p>When enabling 2FA on websites, they typically show:</p>
                <ul style="margin: 10px 0; padding-left: 20px;">
                    <li>A QR code for mobile apps</li>
                    <li>A text secret key (what you need for Selene)</li>
                    <li>Look for "Can't scan QR code?" or "Manual entry" options</li>
                </ul>
            </div>

            <h2 style="color: #e74c3c; border-bottom: 2px solid #e74c3c; padding-bottom: 5px;">🔢 Using Generated Codes</h2>

            <div style="margin: 20px 0;">
                <ul style="margin: 10px 0; padding-left: 20px;">
                    <li>Select an account from the list</li>
                    <li>Current 6-digit code displays in large text</li>
                    <li>Countdown timer shows seconds until next refresh</li>
                    <li>Click "Copy Code to Clipboard" for easy pasting</li>
                    <li>Codes automatically update every 30 seconds</li>
                </ul>
            </div>

            <h2 style="color: #1abc9c; border-bottom: 2px solid #1abc9c; padding-bottom: 5px;">👁️ Visual Indicators</h2>

            <div style="margin: 20px 0;">
                <ul style="margin: 10px 0; padding-left: 20px;">
                    <li>Progress bar shows time remaining (green → yellow → red)</li>
                    <li>Large, easy-to-read monospace font for codes</li>
                    <li>Clear countdown text showing refresh time</li>
                    <li>Copy button changes to "Copied!" for confirmation</li>
                </ul>
            </div>

            <h2 style="color: #e67e22; border-bottom: 2px solid #e67e22; padding-bottom: 5px;">⚙️ Account Management</h2>

            <div style="margin: 20px 0;">
                <ul style="margin: 10px 0; padding-left: 20px;">
                    <li>View all your 2FA accounts in organized list</li>
                    <li>Delete accounts you no longer need</li>
                    <li>Accounts persist between application restarts</li>
                    <li>Secure local storage (not cloud-synced)</li>
                </ul>
            </div>

            <h2 style="color: #8e44ad; border-bottom: 2px solid #8e44ad; padding-bottom: 5px;">🔒 Security Notes</h2>

            <div style="margin: 20px 0;">
                <ul style="margin: 10px 0; padding-left: 20px;">
                    <li>Secret keys are stored locally on your device</li>
                    <li>No internet connection required for code generation</li>
                    <li>Codes are generated using industry-standard TOTP algorithm</li>
                    <li>Same codes as Google Authenticator, Authy, etc.</li>
                </ul>
            </div>

            <h2 style="color: #16a085; border-bottom: 2px solid #16a085; padding-bottom: 5px;">🌐 Supported Services</h2>

            <div style="margin: 20px 0;">
                <p>Works with any service that supports TOTP 2FA:</p>
                <ul style="margin: 10px 0; padding-left: 20px;">
                    <li>Google/Gmail accounts</li>
                    <li>GitHub</li>
                    <li>Discord</li>
                    <li>Microsoft accounts</li>
                    <li>Banking websites</li>
                    <li>Social media platforms</li>
                    <li>And many more</li>
                </ul>
            </div>

            <h2 style="color: #d35400; border-bottom: 2px solid #d35400; padding-bottom: 5px;">📋 Workflow Example</h2>

            <div style="margin: 20px 0;">
                <ol style="margin: 10px 0; padding-left: 25px;">
                    <li>Enable 2FA on GitHub</li>
                    <li>Copy the secret key from GitHub's setup page</li>
                    <li>Add account in Selene's 2FA Manager</li>
                    <li>When logging into GitHub, select the account</li>
                    <li>Copy the current 6-digit code</li>
                    <li>Paste into GitHub's 2FA prompt</li>
                </ol>
            </div>

            <h2 style="color: #7b1fa2; border-bottom: 2px solid #7b1fa2; padding-bottom: 5px;">💡 Benefits Over Mobile Apps</h2>

            <div style="margin: 20px 0;">
                <ul style="margin: 10px 0; padding-left: 20px;">
                    <li>Access codes directly on your computer</li>
                    <li>No need to grab your phone</li>
                    <li>Larger, easier-to-read display</li>
                    <li>Integrated with your browsing workflow</li>
                    <li>Quick clipboard copying</li>
                </ul>
            </div>

            <h2 style="color: #388e3c; border-bottom: 2px solid #388e3c; padding-bottom: 5px;">⏰ Time Synchronization</h2>

            <div style="margin: 20px 0;">
                <ul style="margin: 10px 0; padding-left: 20px;">
                    <li>Codes are time-based (30-second intervals)</li>
                    <li>Uses your system clock for accuracy</li>
                    <li>Same timing as other authenticator apps</li>
                    <li>Automatic refresh every second</li>
                </ul>
            </div>

            <div style="background-color: #d1ecf1; padding: 15px; border-radius: 8px; margin: 25px 0;">
                <h3 style="color: #0c5460; margin-top: 0;">🌟 Integrated Security</h3>
                <p style="margin-bottom: 0;">This 2FA manager eliminates the need for separate authenticator apps while providing the same security benefits, making it convenient to access your two-factor codes directly within Selene.</p>
            </div>
        </div>
    )";
}

QString HelpMenuDialog::getDataManagementContent()
{
    return R"(
        <div style="font-family: Arial, sans-serif; line-height: 1.6; color: #2c3e50;">
            <h1 style="color: #e74c3c; text-align: center; margin-bottom: 20px;">🗂️ Data Management & Privacy</h1>

            <div style="background-color: #ecf0f1; padding: 15px; border-radius: 8px; margin-bottom: 25px;">
                <h2 style="color: #2c3e50; margin-top: 0;">Session & Profile Data Management</h2>
                <p>Selene provides comprehensive tools to manage your browsing data, sessions, and privacy settings. Control what data is stored and when to clear it.</p>
            </div>

            <h2 style="color: #3498db; border-bottom: 2px solid #3498db; padding-bottom: 5px;">🧹 Clean Current Session Data</h2>

            <div style="margin: 20px 0;">
                <p><strong>What it does:</strong></p>
                <p>Clears browsing data from all currently active sessions and the shared profile.</p>

                <p><strong>Data removed:</strong></p>
                <ul style="margin: 10px 0; padding-left: 20px;">
                    <li>All cookies from active sessions</li>
                    <li>HTTP cache from all profiles</li>
                    <li>Visited links history</li>
                    <li>Temporary browsing data</li>
                </ul>

                <p><strong>When to use:</strong></p>
                <ul style="margin: 10px 0; padding-left: 20px;">
                    <li>After browsing sensitive websites</li>
                    <li>When sharing your computer</li>
                    <li>To free up storage space</li>
                    <li>For privacy after online shopping/banking</li>
                </ul>
            </div>

            <h2 style="color: #27ae60; border-bottom: 2px solid #27ae60; padding-bottom: 5px;">👥 Clean Shared Profile Data</h2>

            <div style="margin: 20px 0;">
                <p><strong>What it does:</strong></p>
                <p>Clears browsing data only from the shared profile, leaving separate tab profiles untouched.</p>

                <p><strong>Data removed:</strong></p>
                <ul style="margin: 10px 0; padding-left: 20px;">
                    <li>Shared profile cookies only</li>
                    <li>Shared profile cache</li>
                    <li>Shared profile visited links</li>
                </ul>

                <p><strong>What's preserved:</strong></p>
                <ul style="margin: 10px 0; padding-left: 20px;">
                    <li>Individual tab profile data</li>
                    <li>Private profile sessions</li>
                    <li>Separate profile cookies and cache</li>
                </ul>

                <p><strong>When to use:</strong></p>
                <ul style="margin: 10px 0; padding-left: 20px;">
                    <li>When you want to keep private profiles intact</li>
                    <li>To clear general browsing without affecting work profiles</li>
                    <li>Selective privacy cleaning</li>
                </ul>
            </div>

            <h2 style="color: #e74c3c; border-bottom: 2px solid #e74c3c; padding-bottom: 5px;">🏭 Restore Factory Defaults</h2>

            <div style="margin: 20px 0;">
                <p><strong>What it does:</strong></p>
                <p>Completely resets Selene to its original state, removing all user data and settings.</p>

                <p><strong>Data removed:</strong></p>
                <ul style="margin: 10px 0; padding-left: 20px;">
                    <li>All saved websites and bookmarks</li>
                    <li>All saved sessions</li>
                    <li>All application settings and preferences</li>
                    <li>Security settings and master passwords</li>
                    <li>All browsing data (cookies, cache, history)</li>
                    <li>Application data directories</li>
                    <li>Profile configurations</li>
                </ul>
            </div>

            <div style="background-color: #f8d7da; padding: 15px; border-radius: 8px; margin: 15px 0; border-left: 4px solid #dc3545;">
                <h3 style="color: #721c24; margin-top: 0;">⚠️ Factory Reset Warning</h3>
                <p style="margin-bottom: 0;"><strong>This action cannot be undone!</strong></p>
                <ul style="margin: 10px 0; padding-left: 20px;">
                    <li>All your saved data will be permanently lost</li>
                    <li>Application will close automatically after reset</li>
                    <li>You'll need to restart Selene manually</li>
                    <li>All customizations will be lost</li>
                </ul>
            </div>

            <h2 style="color: #f39c12; border-bottom: 2px solid #f39c12; padding-bottom: 5px;">📍 How to Access These Features</h2>

            <div style="margin: 20px 0;">
                <p>All data management options are located in the <strong>Sessions</strong> menu:</p>
                <ol style="margin: 10px 0; padding-left: 25px;">
                    <li>Click on "Sessions" in the menu bar</li>
                    <li>Scroll to the bottom section</li>
                    <li>Choose your desired cleaning option</li>
                    <li>Confirm the action in the dialog box</li>
                </ol>
            </div>

            <h2 style="color: #9b59b6; border-bottom: 2px solid #9b59b6; padding-bottom: 5px;">🔄 Data Types Explained</h2>

            <div style="margin: 20px 0;">
                <h3 style="color: #8e44ad;">Cookies</h3>
                <ul style="margin: 10px 0; padding-left: 20px;">
                    <li>Login sessions and preferences</li>
                    <li>Shopping cart contents</li>
                    <li>Website customizations</li>
                </ul>

                <h3 style="color: #8e44ad;">HTTP Cache</h3>
                <ul style="margin: 10px 0; padding-left: 20px;">
                    <li>Temporarily stored website files</li>
                    <li>Images, scripts, and stylesheets</li>
                    <li>Speeds up repeat visits</li>
                </ul>

                <h3 style="color: #8e44ad;">Visited Links</h3>
                <ul style="margin: 10px 0; padding-left: 20px;">
                    <li>History of visited websites</li>
                    <li>Link color changes (visited vs unvisited)</li>
                    <li>Navigation history</li>
                </ul>
            </div>

            <h2 style="color: #1abc9c; border-bottom: 2px solid #1abc9c; padding-bottom: 5px;">🛡️ Privacy Recommendations</h2>

            <div style="margin: 20px 0;">
                <h3 style="color: #16a085;">Regular Cleaning (Weekly)</h3>
                <ul style="margin: 10px 0; padding-left: 20px;">
                    <li>Use "Clean Shared Profile Data" for routine maintenance</li>
                    <li>Keeps private profiles intact</li>
                    <li>Maintains good performance</li>
                </ul>

                <h3 style="color: #16a085;">Deep Cleaning (Monthly)</h3>
                <ul style="margin: 10px 0; padding-left: 20px;">
                    <li>Use "Clean Current Session Data" for thorough cleanup</li>
                    <li>Clears all active session data</li>
                    <li>Good for privacy and storage</li>
                </ul>

                <h3 style="color: #16a085;">Emergency Cleaning</h3>
                <ul style="margin: 10px 0; padding-left: 20px;">
                    <li>After using public computers</li>
                    <li>When selling or giving away device</li>
                    <li>After security concerns</li>
                </ul>
            </div>

            <h2 style="color: #e67e22; border-bottom: 2px solid #e67e22; padding-bottom: 5px;">💡 Smart Usage Tips</h2>

            <div style="margin: 20px 0;">
                <ul style="margin: 10px 0; padding-left: 20px;">
                    <li><strong>Before cleaning:</strong> Save any important sessions you want to keep</li>
                    <li><strong>Profile separation:</strong> Use private profiles for sensitive browsing</li>
                    <li><strong>Regular maintenance:</strong> Clean shared profile weekly, all data monthly</li>
                    <li><strong>Factory reset:</strong> Only use when starting completely fresh</li>
                    <li><strong>Backup important data:</strong> Export sessions before major cleaning</li>
                </ul>
            </div>

            <h2 style="color: #8e44ad; border-bottom: 2px solid #8e44ad; padding-bottom: 5px;">⚡ Performance Benefits</h2>

            <div style="margin: 20px 0;">
                <p><strong>Regular data cleaning provides:</strong></p>
                <ul style="margin: 10px 0; padding-left: 20px;">
                    <li>Faster application startup</li>
                    <li>Reduced memory usage</li>
                    <li>More available storage space</li>
                    <li>Improved browsing performance</li>
                    <li>Better privacy protection</li>
                </ul>
            </div>

            <div style="background-color: #d1ecf1; padding: 15px; border-radius: 8px; margin: 25px 0;">
                <h3 style="color: #0c5460; margin-top: 0;">🎯 Choose the Right Tool</h3>
                <p style="margin-bottom: 0;">
                    <strong>Shared Profile Clean:</strong> For routine maintenance<br>
                    <strong>Current Session Clean:</strong> For thorough privacy cleaning<br>
                    <strong>Factory Reset:</strong> For complete fresh start
                </p>
            </div>
        </div>
    )";
}

QString HelpMenuDialog::getDownloadManagerContent()
{
    return R"(
        <div style="font-family: Arial, sans-serif; line-height: 1.6; color: #2c3e50;">
            <h1 style="color: #e74c3c; text-align: center; margin-bottom: 20px;">📥 Download Manager</h1>

            <div style="background-color: #ecf0f1; padding: 15px; border-radius: 8px; margin-bottom: 25px;">
                <h2 style="color: #2c3e50; margin-top: 0;">Integrated Download Management</h2>
                <p>Selene includes a comprehensive download manager that handles all your file downloads with progress tracking, organization, and easy access to downloaded files.</p>
            </div>

            <h2 style="color: #3498db; border-bottom: 2px solid #3498db; padding-bottom: 5px;">🚀 Key Features</h2>

            <div style="margin: 20px 0;">
                <ul style="margin: 10px 0; padding-left: 20px;">
                    <li>Real-time download progress tracking</li>
                    <li>Download speed and time remaining calculations</li>
                    <li>Automatic file organization in dedicated folder</li>
                    <li>Duplicate filename handling</li>
                    <li>One-click access to files and folders</li>
                    <li>Download history management</li>
                    <li>Cancel active downloads</li>
                    <li>Clean interface with visual progress bars</li>
                </ul>
            </div>

            <h2 style="color: #27ae60; border-bottom: 2px solid #27ae60; padding-bottom: 5px;">📍 How to Access Downloads</h2>

            <div style="margin: 20px 0;">
                <h3 style="color: #229954;">Opening the Download Manager</h3>
                <ul style="margin: 10px 0; padding-left: 20px;">
                    <li>Click the <strong>Downloads</strong> icon in the toolbar</li>
                    <li>Or go to <strong>View → Downloads</strong> in the menu</li>
                    <li>Download window opens showing all current and past downloads</li>
                </ul>

                <h3 style="color: #229954;">Download Location</h3>
                <ul style="margin: 10px 0; padding-left: 20px;">
                    <li>Files are saved to: <code>Downloads/Selene/</code></li>
                    <li>Organized in your system's default Downloads folder</li>
                    <li>Automatic folder creation if it doesn't exist</li>
                </ul>
            </div>

            <h2 style="color: #f39c12; border-bottom: 2px solid #f39c12; padding-bottom: 5px;">📊 Download Progress Tracking</h2>

            <div style="margin: 20px 0;">
                <h3 style="color: #d68910;">Real-time Information</h3>
                <ul style="margin: 10px 0; padding-left: 20px;">
                    <li><strong>File name and size:</strong> Clear identification of what's downloading</li>
                    <li><strong>Progress bar:</strong> Visual representation of download completion</li>
                    <li><strong>Speed indicator:</strong> Current download speed (KB/s, MB/s)</li>
                    <li><strong>Time remaining:</strong> Estimated completion time</li>
                    <li><strong>Status updates:</strong> Starting, downloading, completed, cancelled</li>
                </ul>

                <h3 style="color: #d68910;">Progress Display</h3>
                <ul style="margin: 10px 0; padding-left: 20px;">
                    <li>Percentage completion with visual progress bar</li>
                    <li>Downloaded size vs. total file size</li>
                    <li>Dynamic speed calculations updated every second</li>
                    <li>Color-coded status indicators</li>
                </ul>
            </div>

            <h2 style="color: #9b59b6; border-bottom: 2px solid #9b59b6; padding-bottom: 5px;">🎛️ Download Controls</h2>

            <div style="margin: 20px 0;">
                <h3 style="color: #8e44ad;">During Download</h3>
                <ul style="margin: 10px 0; padding-left: 20px;">
                    <li><strong>Cancel Button:</strong> Stop active downloads immediately</li>
                    <li><strong>Open Folder:</strong> Access download directory anytime</li>
                    <li><strong>Progress Monitoring:</strong> Watch real-time progress</li>
                </ul>

                <h3 style="color: #8e44ad;">After Download</h3>
                <ul style="margin: 10px 0; padding-left: 20px;">
                    <li><strong>Open File:</strong> Launch downloaded file directly</li>
                    <li><strong>Open Folder:</strong> Navigate to file location</li>
                    <li><strong>Remove from List:</strong> Clean up download history</li>
                </ul>
            </div>

            <h2 style="color: #1abc9c; border-bottom: 2px solid #1abc9c; padding-bottom: 5px;">🗂️ File Organization</h2>

            <div style="margin: 20px 0;">
                <h3 style="color: #16a085;">Automatic Organization</h3>
                <ul style="margin: 10px 0; padding-left: 20px;">
                    <li>All downloads saved to dedicated Selene folder</li>
                    <li>Automatic duplicate filename handling</li>
                    <li>Files renamed with numbers: <code>file.pdf</code>, <code>file (1).pdf</code>, <code>file (2).pdf</code></li>
                    <li>Preserves original file extensions</li>
                </ul>

                <h3 style="color: #16a085;">Smart Naming</h3>
                <ul style="margin: 10px 0; padding-left: 20px;">
                    <li>Uses original filename from website</li>
                    <li>Fallback to "download" if no name available</li>
                    <li>Prevents file overwrites automatically</li>
                </ul>
            </div>

            <h2 style="color: #e67e22; border-bottom: 2px solid #e67e22; padding-bottom: 5px;">🧹 Download Management</h2>

            <div style="margin: 20px 0;">
                <h3 style="color: #d35400;">Window Controls</h3>
                <ul style="margin: 10px 0; padding-left: 20px;">
                    <li><strong>Clear Finished:</strong> Remove completed/cancelled downloads from list</li>
                    <li><strong>Open Downloads Folder:</strong> Quick access to download directory</li>
                    <li><strong>Individual Remove:</strong> Remove specific items from history</li>
                </ul>

                <h3 style="color: #d35400;">List Management</h3>
                <ul style="margin: 10px 0; padding-left: 20px;">
                    <li>Chronological list of all downloads</li>
                    <li>Persistent across application restarts</li>
                    <li>Easy cleanup of old downloads</li>
                    <li>Empty state message when no downloads</li>
                </ul>
            </div>

            <h2 style="color: #8e44ad; border-bottom: 2px solid #8e44ad; padding-bottom: 5px;">💡 Download States</h2>

            <div style="margin: 20px 0;">
                <h3 style="color: #7b1fa2;">Active Downloads</h3>
                <ul style="margin: 10px 0; padding-left: 20px;">
                    <li><strong>Starting:</strong> Download initializing</li>
                    <li><strong>In Progress:</strong> Actively downloading with progress</li>
                    <li><strong>Speed Display:</strong> Real-time transfer rate</li>
                    <li><strong>Cancel Option:</strong> Stop button available</li>
                </ul>

                <h3 style="color: #7b1fa2;">Completed Downloads</h3>
                <ul style="margin: 10px 0; padding-left: 20px;">
                    <li><strong>Completed:</strong> Successfully downloaded</li>
                    <li><strong>Cancelled:</strong> User stopped download</li>
                    <li><strong>Interrupted:</strong> Network or system error</li>
                    <li><strong>File Access:</strong> Open file/folder buttons available</li>
                </ul>
            </div>

            <h2 style="color: #d32f2f; border-bottom: 2px solid #d32f2f; padding-bottom: 5px;">⚡ Performance Features</h2>

            <div style="margin: 20px 0;">
                <ul style="margin: 10px 0; padding-left: 20px;">
                    <li><strong>Efficient Memory Usage:</strong> Minimal resource consumption</li>
                    <li><strong>Background Downloads:</strong> Continue while browsing</li>
                    <li><strong>Multiple Downloads:</strong> Handle several files simultaneously</li>
                    <li><strong>Speed Calculation:</strong> Accurate transfer rate monitoring</li>
                    <li><strong>Progress Updates:</strong> Smooth, real-time progress tracking</li>
                </ul>
            </div>

            <h2 style="color: #388e3c; border-bottom: 2px solid #388e3c; padding-bottom: 5px;">🔧 Technical Details</h2>

            <div style="margin: 20px 0;">
                <h3 style="color: #2e7d32;">File Size Formatting</h3>
                <ul style="margin: 10px 0; padding-left: 20px;">
                    <li>Automatic unit conversion: B → KB → MB → GB</li>
                    <li>Decimal precision for readability</li>
                    <li>Speed shown as size per second</li>
                </ul>

                <h3 style="color: #2e7d32;">Time Calculations</h3>
                <ul style="margin: 10px 0; padding-left: 20px;">
                    <li>Remaining time based on current speed</li>
                    <li>Format: seconds, minutes, hours as appropriate</li>
                    <li>Dynamic updates as speed changes</li>
                </ul>
            </div>

            <h2 style="color: #5d4037; border-bottom: 2px solid #5d4037; padding-bottom: 5px;">🎯 Usage Tips</h2>

            <div style="margin: 20px 0;">
                <ul style="margin: 10px 0; padding-left: 20px;">
                    <li><strong>Monitor Progress:</strong> Keep download window open to watch progress</li>
                    <li><strong>Multiple Downloads:</strong> Start several downloads simultaneously</li>
                    <li><strong>Quick Access:</strong> Use "Open Folder" for easy file management</li>
                    <li><strong>Clean History:</strong> Regularly clear finished downloads</li>
                    <li><strong>Cancel if Needed:</strong> Stop unwanted downloads immediately</li>
                    <li><strong>File Organization:</strong> Downloads are automatically organized</li>
                </ul>
            </div>

            <div style="background-color: #fff3cd; padding: 15px; border-radius: 8px; margin: 15px 0;">
                <h3 style="color: #856404; margin-top: 0;">📁 Platform-Specific Notes</h3>
                <p><strong>Flathub Version:</strong> Shows download location in dialog</p>
                <p><strong>Standard Version:</strong> Opens file manager directly</p>
                <p style="margin-bottom: 0;"><strong>All Platforms:</strong> Downloads saved to system Downloads folder under "Selene" subdirectory</p>
            </div>

            <div style="background-color: #d1ecf1; padding: 15px; border-radius: 8px; margin: 25px 0;">
                <h3 style="color: #0c5460; margin-top: 0;">🌟 Integrated Experience</h3>
                <p style="margin-bottom: 0;">The download manager seamlessly integrates with your browsing experience, automatically handling all file downloads while providing full control and visibility over the download process.</p>
            </div>
        </div>
    )";
}

QString HelpMenuDialog::getSitesSessionsContent()
{
    return QString(
        "<h3>Managing Sites</h3>"
        "<p><strong>Creating a New Site:</strong></p>"
        "<ul>"
        "<li>Press the <strong>Clear</strong> button to clear existing fields</li>"
        "<li>Fill in the <strong>URL</strong> and <strong>Title</strong> (required fields)</li>"
        "<li>Optionally add <strong>Username</strong>, <strong>Password</strong>, and <strong>Comments</strong></li>"
        "<li>Press the <strong>Add</strong> button to save the site</li>"
        "</ul>"

        "<h3>Managing Sessions</h3>"
        "<p><strong>Creating a New Session:</strong></p>"
        "<ul>"
        "<li>Ensure at least one tab is open in the webview</li>"
        "<li>Select <strong>Save Current Session</strong> from the menu or toolbar</li>"
        "<li>Give your session a name and click <strong>OK</strong></li>"
        "<li>Sessions are automatically assigned a randomly generated SVG icon</li>"
        "</ul>"

        "<h3>Editing Sites and Sessions</h3>"
        "<p><strong>To edit any site or session:</strong></p>"
        "<ul>"
        "<li>Select the item you want to modify</li>"
        "<li>Enter the new values in the appropriate fields</li>"
        "<li>Press the <strong>Update</strong> button to save changes</li>"
        "<li>For sessions: Click the small <strong>Edit</strong> button next to the icon to change it, then click <strong>Update</strong></li>"
        "</ul>"
        );
}

QString HelpMenuDialog::getOnSecurityContent()
{
    return QString(
        "<h3>Security Features Overview</h3>"
        "<p>Selene provides several optional security features designed for your convenience. "
        "You are completely free to use or not use any of these features based on your preferences.</p>"

        "<h4>Username & Password Storage</h4>"
        "<ul>"
        "<li>Storing credentials in website entries is <strong>completely optional</strong></li>"
        "<li>Leave these fields blank if you prefer to use your own credentials manager</li>"
        "<li>Stored credentials are saved locally on your device only in binary format</li>"
        "<li>No data is transmitted over the network</li>"
        "</ul>"

        "<h4>Master Password Protection</h4>"
        "<ul>"
        "<li>Optional feature to protect access to Selene</li>"
        "<li>When enabled, you'll need to enter your master password on startup</li>"
        "<li>Choose a strong, memorable password and store it safely</li>"
        "<li>If forgotten, you'll need to perform a factory reset</li>"
        "</ul>"

        "<h4>Two-Factor Authentication (2FA) Manager</h4>"
        "<ul>"
        "<li>Optional convenience tool for generating TOTP codes</li>"
        "<li>Helps manage 2FA codes for your various accounts</li>"
        "<li>All secrets are stored locally on your device in binary format</li>"
        "<li>Use only if you're comfortable with local storage</li>"
        "</ul>"

        "<h4>Security Disclaimer</h4>"
        "<p><em>While every reasonable effort has been made to implement a secure framework "
        "and all sensitive info is stored in binary format within Selene, "
        "these features are provided as conveniences rather than guarantees. Users are responsible "
        "for deciding what information to store based on their individual security requirements and risk tolerance.</em></p>"

        "<p><strong>Recommendation:</strong> For maximum security, consider using dedicated password managers "
        "and letting your browser handle credential storage.</p>"
        );
}

QString HelpMenuDialog::getOnNewStorageSystemContent() {
    return QString(
        "<h3>New Storage System (Selene 1.1.0 and onwards)</h3>"
        "<p>This version of Selene uses a new storage system that improves performance and efficiency by using symlinks for profile data. This means that instead of copying entire profile directories, only links are created, saving disk space and speeding up operations.</p>"
        "<h4>Recommendations:</h4>"
        "<p>To fully leverage the new storage system and ensure optimal performance, we highly recommend performing a factory reset. This will clear all old data and provide a clean start. You can find the factory reset option in the 'Sessions' menu.</p>"
        "<h4>Clean up orphaned profile directories on startup:</h4>"
        "<p>As part of the new storage system, a checkbox is available in the 'Tools' menu to automatically clean up unused, orphaned profile directories at application startup. It is STRONGLY RECOMMENDED to keep this option enabled UNLESS you also have old-format sessions saved.</p>"
        "<h4>Handling Old-Format Sessions:</h4>"
        "<p>If you have existing sessions that use the old directory-based storage, you can continue to use them. However, we strongly advise against adding new tabs or saving changes to these older sessions. To ensure optimal performance and avoid potential issues, it's best to create new sessions using the new storage system.</p>"
        );
}

QString HelpMenuDialog::getOnNamedProfilesContent()
{
    return QString(R"(
        <div style="text-align: center; font-family: Arial, sans-serif;">
            <h1 style="color: #2c3e50; margin-bottom: 10px;">Named Shared Profiles</h1>
            <h3 style="color: #7f8c8d; margin-bottom: 20px;">Contextual Browsing Environments</h3>

            <div style="text-align: left; margin: 20px 0; line-height: 1.6;">
                <p>
                    Selene's Named Shared Profiles feature takes your browsing organization to the next level by allowing
                    multiple tabs to share the same browsing context under meaningful labels like "Work," "Home," "Shopping,"
                    or "Research." This powerful addition bridges the gap between completely isolated private profiles and a
                    single shared environment.
                </p>

                <div style="background-color: #ecf0f1; padding: 15px; border-radius: 8px; margin: 20px 0;">
                    <h4 style="color: #2c3e50; margin-top: 0;">Examples:</h4>
                    <ul style="margin-left: 20px;">
                        <li><strong>Work Profile:</strong> Company email, project management tools, and internal documentation all share cookies and login states</li>
                        <li><strong>Personal Profile:</strong> Social media and entertainment sites kept separate from work accounts</li>
                        <li><strong>Finance Profile:</strong> Banking, investment, and budgeting tools with shared authentication</li>
                        <li><strong>Travel Profile:</strong> Airline, hotel, and rental car sites that need to exchange booking information</li>
                    </ul>
                </div>

                <p>
                    This contextual grouping maintains the perfect balance between isolation and integration: your work accounts
                    never mix with personal browsing, but related tools within each context can seamlessly communicate when needed.
                    Named Shared Profiles eliminate the all-or-nothing approach to profile isolation, giving you granular control
                    over exactly which tabs should share data with each other while maintaining barriers between different areas of your digital life.
                </p>

                <h3 style="color: #2c3e50; margin-top: 30px;">How to Use Named Profiles</h3>

                <ol style="margin-left: 20px;">
                    <li><strong>Create Profiles:</strong> In the profile area of the toolbar, in the profileSelector combobox select "New Profile..." and give it a meaningful name like "Work" or "Personal"</li>
                    <li><strong>Select a Profile:</strong> Choose your desired profile from the dropdown before launching websites</li>
                    <li><strong>Launch Websites:</strong> Any sites launched while a named profile is selected will share the same browsing context</li>
                    <li><strong>Create Sessions:</strong> Save groups of tabs launched under the same named profile for quick restoration</li>
                    <li><strong>Manage Profiles:</strong> Select "Manage Profiles..." to create, delete, or clean profile data</li>
                </ol>

                <div style="background-color: #e8f4f8; padding: 15px; border-radius: 8px; margin: 20px 0; border-left: 4px solid #3498db;">
                <p><strong>Note:</strong> Keeping the profile selector set to "Default" will use the universal shared profile,
                which is the standard shared browsing environment. Private profiles (toggled per tab via the Private Toggle Button) always remain
                completely isolated regardless of named profile selection.</p>

                <p><strong>Simply put:</strong></p>
                <ul style="margin-left: 20px;">
                    <li>To create tabs with private profile, toggle the Private Profile button ON in the toolbar.
                    This will inactivate the Named Profile selector.</li>
                    <li>To use a named shared profile, untoggle the Private Profile button (if toggled) and select a previously
                    created named shared profile from the combobox.</li>
                    <li>Or leave it set to "Default" to use the universal profile.</li>
                </ul>
                </div>



                <h3 style="color: #2c3e50; margin-top: 30px;">Profile Management</h3>
                <p>
                    You can manage your named profiles by selecting "Manage Profiles..." from the profile selector dropdown. This allows you to:
                </p>
                <ul style="margin-left: 20px;">
                    <li><strong>Create new profiles</strong> for different contexts or projects</li>
                    <li><strong>Delete profiles</strong> you no longer need (only if they're not in use by active sessions)</li>
                    <li><strong>Clean profile data</strong> to remove cookies, cache, and browsing history while keeping the profile</li>
                </ul>

                <p>
                    Each named profile maintains its own separate storage for:
                </p>
                <ul style="margin-left: 20px;">
                    <li>Cookies and login sessions</li>
                    <li>Browsing history and cache</li>
                    <li>Local storage and website data</li>
                    <li>Form data and preferences</li>
                </ul>
            </div>

            <hr style="margin: 30px 0; border: 1px solid #bdc3c7;">

            <div style="margin: 20px 0;">
                <p style="font-size: 14px; color: #7f8c8d;">
                    Organize your digital life with contextual browsing environments
                </p>
            </div>
        </div>
    )");

}




