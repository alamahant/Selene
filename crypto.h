#ifndef CRYPTO_H
#define CRYPTO_H
#include<QObject>

#include <QString>
#include <QByteArray>

// Forward declaration for OpenSSL types (if needed)
typedef struct evp_pkey_st EVP_PKEY;

class Crypto
{
public:
    Crypto();
    ~Crypto();

    // Key management
    bool generateKeyPair(int rsaBits, bool force = false); // Generates and saves keys if not present
    bool loadKeys();        // Loads keys from files
    bool saveKeys();        // Saves keys to files

    // Encryption/Decryption
    // Encrypts plainText with peer's public key (PEM format as QString), returns true on success
    bool encrypt(const QByteArray &plainText, const QString &peerPubKeyPem, QByteArray &cipherText);
    // Decrypts cipherText with own private key, returns true on success
    bool decrypt(const QByteArray &cipherText, QByteArray &plainText);

    // Public key accessor (PEM format)
    QString getPublicKey() const;

    // Utility
    bool keysExist() const; // Checks if both key files exist and are valid

private:
    QString privateKeyPath;
    QString publicKeyPath;

    EVP_PKEY* privateKey; // OpenSSL key handle
    EVP_PKEY* publicKey;  // OpenSSL key handle

    // Loads/saves keys from/to files in getCryptoDirPath()
    bool loadPrivateKey();
    bool loadPublicKey();
    bool savePrivateKey();
    bool savePublicKey();

    // Helper to get the key storage directory
    QString getCryptoDirPath() const;

    // Disable copy
    Crypto(const Crypto&) = delete;
    Crypto& operator=(const Crypto&) = delete;
};



#endif // CRYPTO_H
