#include "crypto.h"

#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QDebug>

#include <openssl/pem.h>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/err.h>
#include"constants.h"

QString getDefaultCryptoDirPath() {
    QString cryptoDir = getCryptoDirPath();
    QDir().mkpath(cryptoDir);
    return cryptoDir;
}


Crypto::Crypto()
    : privateKey(nullptr), publicKey(nullptr)
{
    QString cryptoDir = getCryptoDirPath();
    privateKeyPath = cryptoDir + QDir::separator() + "private.pem";
    publicKeyPath  = cryptoDir + QDir::separator() + "public.pem";
    loadKeys();
}

Crypto::~Crypto()
{
    if (privateKey) EVP_PKEY_free(privateKey);
    if (publicKey) EVP_PKEY_free(publicKey);
}

QString Crypto::getCryptoDirPath() const
{
    return getDefaultCryptoDirPath();
}

bool Crypto::keysExist() const
{
    QFile privFile(privateKeyPath);
    QFile pubFile(publicKeyPath);
    return privFile.exists() && pubFile.exists();
}


bool Crypto::generateKeyPair(int rsaBits, bool force)
{
    if (keysExist()) {
        if (!force) {
            return true; // Do not overwrite existing keys
        }
        // Remove existing keys if force is true
        QFile::remove(privateKeyPath);
        QFile::remove(publicKeyPath);
    }

    // Create context for RSA key generation
    EVP_PKEY_CTX *pctx = EVP_PKEY_CTX_new_id(EVP_PKEY_RSA, nullptr);
    if (!pctx) return false;

    EVP_PKEY *pkey = nullptr;
    if (EVP_PKEY_keygen_init(pctx) <= 0) {
        EVP_PKEY_CTX_free(pctx);
        return false;
    }

    // Set RSA key size (2048 bits)
    if (EVP_PKEY_CTX_set_rsa_keygen_bits(pctx, rsaBits) <= 0) {
        EVP_PKEY_CTX_free(pctx);
        return false;
    }

    if (EVP_PKEY_keygen(pctx, &pkey) <= 0) {
        EVP_PKEY_CTX_free(pctx);
        return false;
    }

    EVP_PKEY_CTX_free(pctx);

    // Save private key
    QFile privFile(privateKeyPath);
    if (!privFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        EVP_PKEY_free(pkey);
        return false;
    }

    BIO *privBio = BIO_new(BIO_s_mem());
    if (!PEM_write_bio_PrivateKey(privBio, pkey, nullptr, nullptr, 0, nullptr, nullptr)) {
        BIO_free(privBio);
        EVP_PKEY_free(pkey);
        return false;
    }

    BUF_MEM *privMem;
    BIO_get_mem_ptr(privBio, &privMem);
    privFile.write(privMem->data, privMem->length);
    privFile.setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner);
    privFile.close();
    BIO_free(privBio);

    // Save public key
    QFile pubFile(publicKeyPath);
    if (!pubFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        EVP_PKEY_free(pkey);
        return false;
    }

    BIO *pubBio = BIO_new(BIO_s_mem());
    if (!PEM_write_bio_PUBKEY(pubBio, pkey)) {
        BIO_free(pubBio);
        EVP_PKEY_free(pkey);
        return false;
    }

    BUF_MEM *pubMem;
    BIO_get_mem_ptr(pubBio, &pubMem);
    pubFile.write(pubMem->data, pubMem->length);
    pubFile.close();
    BIO_free(pubBio);

    EVP_PKEY_free(pkey);


    // Reload keys into memory
    return loadKeys();
}



bool Crypto::loadKeys()
{
    bool privOk = loadPrivateKey();
    bool pubOk = loadPublicKey();
    return privOk && pubOk;
}

bool Crypto::loadPrivateKey()
{
    if (privateKey) {
        EVP_PKEY_free(privateKey);
        privateKey = nullptr;
    }
    QFile privFile(privateKeyPath);
    if (!privFile.open(QIODevice::ReadOnly)) return false;
    QByteArray privData = privFile.readAll();
    BIO *bio = BIO_new_mem_buf(privData.data(), privData.size());
    privateKey = PEM_read_bio_PrivateKey(bio, nullptr, nullptr, nullptr);
    BIO_free(bio);
    return privateKey != nullptr;
}

bool Crypto::loadPublicKey()
{
    if (publicKey) {
        EVP_PKEY_free(publicKey);
        publicKey = nullptr;
    }
    QFile pubFile(publicKeyPath);
    if (!pubFile.open(QIODevice::ReadOnly)) return false;
    QByteArray pubData = pubFile.readAll();
    BIO *bio = BIO_new_mem_buf(pubData.data(), pubData.size());
    publicKey = PEM_read_bio_PUBKEY(bio, nullptr, nullptr, nullptr);
    BIO_free(bio);
    return publicKey != nullptr;
}

bool Crypto::saveKeys()
{
    // Not typically needed, as keys are saved on generation
    // Provided for completeness
    if (!privateKey || !publicKey) return false;

    // Save private key
    QFile privFile(privateKeyPath);
    if (!privFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) return false;
    BIO *privBio = BIO_new(BIO_s_mem());
    if (!PEM_write_bio_PrivateKey(privBio, privateKey, nullptr, nullptr, 0, nullptr, nullptr)) {
        BIO_free(privBio);
        return false;
    }
    BUF_MEM *privMem;
    BIO_get_mem_ptr(privBio, &privMem);
    privFile.write(privMem->data, privMem->length);
    privFile.setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner);
    privFile.close();
    BIO_free(privBio);

    // Save public key
    QFile pubFile(publicKeyPath);
    if (!pubFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) return false;
    BIO *pubBio = BIO_new(BIO_s_mem());
    if (!PEM_write_bio_PUBKEY(pubBio, publicKey)) {
        BIO_free(pubBio);
        return false;
    }
    BUF_MEM *pubMem;
    BIO_get_mem_ptr(pubBio, &pubMem);
    pubFile.write(pubMem->data, pubMem->length);
    pubFile.close();
    BIO_free(pubBio);

    return true;
}

QString Crypto::getPublicKey() const
{
    QFile pubFile(publicKeyPath);
    if (!pubFile.open(QIODevice::ReadOnly)) return QString();
    QTextStream in(&pubFile);
    return in.readAll();
}

bool Crypto::encrypt(const QByteArray &plainText, const QString &peerPubKeyPem, QByteArray &cipherText)
{
    // Load peer's public key from PEM string
    QByteArray pubKeyBytes = peerPubKeyPem.toUtf8();
    BIO *bio = BIO_new_mem_buf(pubKeyBytes.data(), pubKeyBytes.size());
    EVP_PKEY *peerKey = PEM_read_bio_PUBKEY(bio, nullptr, nullptr, nullptr);
    BIO_free(bio);
    if (!peerKey) return false;

    EVP_PKEY_CTX *ctx = EVP_PKEY_CTX_new(peerKey, nullptr);
    if (!ctx) {
        EVP_PKEY_free(peerKey);
        return false;
    }
    if (EVP_PKEY_encrypt_init(ctx) <= 0) {
        EVP_PKEY_CTX_free(ctx);
        EVP_PKEY_free(peerKey);
        return false;
    }

    size_t outlen = 0;
    if (EVP_PKEY_encrypt(ctx, nullptr, &outlen, reinterpret_cast<const unsigned char*>(plainText.data()), plainText.size()) <= 0) {
        EVP_PKEY_CTX_free(ctx);
        EVP_PKEY_free(peerKey);
        return false;
    }
    QByteArray outBuf;
    outBuf.resize(outlen);
    if (EVP_PKEY_encrypt(ctx, reinterpret_cast<unsigned char*>(outBuf.data()), &outlen, reinterpret_cast<const unsigned char*>(plainText.data()), plainText.size()) <= 0) {
        EVP_PKEY_CTX_free(ctx);
        EVP_PKEY_free(peerKey);
        return false;
    }
    outBuf.resize(outlen);
    cipherText = outBuf;

    EVP_PKEY_CTX_free(ctx);
    EVP_PKEY_free(peerKey);
    return true;
}

bool Crypto::decrypt(const QByteArray &cipherText, QByteArray &plainText)
{
    if (!privateKey) return false;

    EVP_PKEY_CTX *ctx = EVP_PKEY_CTX_new(privateKey, nullptr);
    if (!ctx) return false;
    if (EVP_PKEY_decrypt_init(ctx) <= 0) {
        EVP_PKEY_CTX_free(ctx);
        return false;
    }

    size_t outlen = 0;
    if (EVP_PKEY_decrypt(ctx, nullptr, &outlen, reinterpret_cast<const unsigned char*>(cipherText.data()), cipherText.size()) <= 0) {
        EVP_PKEY_CTX_free(ctx);
        return false;
    }
    QByteArray outBuf;
    outBuf.resize(outlen);
    if (EVP_PKEY_decrypt(ctx, reinterpret_cast<unsigned char*>(outBuf.data()), &outlen, reinterpret_cast<const unsigned char*>(cipherText.data()), cipherText.size()) <= 0) {
        EVP_PKEY_CTX_free(ctx);
        return false;
    }
    outBuf.resize(outlen);
    plainText = outBuf;

    EVP_PKEY_CTX_free(ctx);
    return true;
}

//AES
QByteArray Crypto::generateAESKey(int size)
{
    QByteArray key(size, '\0');
    if (RAND_bytes(reinterpret_cast<unsigned char*>(key.data()), size) != 1) {
        qWarning() << "Failed to generate AES key";
        return QByteArray();
    }
    return key;
}

QByteArray Crypto::encryptAES(const QByteArray &data, const QByteArray &key, QByteArray &iv)
{
    iv = QByteArray(16, '\0'); // AES block size = 16
    if (RAND_bytes(reinterpret_cast<unsigned char*>(iv.data()), iv.size()) != 1) {
        qWarning() << "Failed to generate IV";
        return QByteArray();
    }

    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    QByteArray cipher(data.size() + 16, '\0'); // extra space for padding
    int outLen1 = 0, outLen2 = 0;

    if (!ctx) return QByteArray();

    if (1 != EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), nullptr,
                                reinterpret_cast<const unsigned char*>(key.data()),
                                reinterpret_cast<const unsigned char*>(iv.data()))) {
        EVP_CIPHER_CTX_free(ctx);
        return QByteArray();
    }

    if (1 != EVP_EncryptUpdate(ctx,
                               reinterpret_cast<unsigned char*>(cipher.data()), &outLen1,
                               reinterpret_cast<const unsigned char*>(data.data()), data.size())) {
        EVP_CIPHER_CTX_free(ctx);
        return QByteArray();
    }

    if (1 != EVP_EncryptFinal_ex(ctx,
                                 reinterpret_cast<unsigned char*>(cipher.data()) + outLen1, &outLen2)) {
        EVP_CIPHER_CTX_free(ctx);
        return QByteArray();
    }

    EVP_CIPHER_CTX_free(ctx);
    cipher.resize(outLen1 + outLen2);
    return cipher;
}

QByteArray Crypto::decryptAES(const QByteArray &cipher, const QByteArray &key, const QByteArray &iv)
{
    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    QByteArray plain(cipher.size(), '\0');
    int outLen1 = 0, outLen2 = 0;

    if (!ctx) return QByteArray();

    if (1 != EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), nullptr,
                                reinterpret_cast<const unsigned char*>(key.data()),
                                reinterpret_cast<const unsigned char*>(iv.data()))) {
        EVP_CIPHER_CTX_free(ctx);
        return QByteArray();
    }

    if (1 != EVP_DecryptUpdate(ctx,
                               reinterpret_cast<unsigned char*>(plain.data()), &outLen1,
                               reinterpret_cast<const unsigned char*>(cipher.data()), cipher.size())) {
        EVP_CIPHER_CTX_free(ctx);
        return QByteArray();
    }

    if (1 != EVP_DecryptFinal_ex(ctx,
                                 reinterpret_cast<unsigned char*>(plain.data()) + outLen1, &outLen2)) {
        EVP_CIPHER_CTX_free(ctx);
        return QByteArray();
    }

    EVP_CIPHER_CTX_free(ctx);
    plain.resize(outLen1 + outLen2);
    return plain;
}
