// src/core/secretstore_fallback.cpp
// 后备后端（Linux / Android）/ Fallback backend (Linux / Android).
//
// 不再是裸 Base64（可逆、等效明文）。改用 Qt 内置能力做认证加密：
//   key    = PBKDF2-HMAC-SHA256(appPepper, salt, 120000, 32)
//   stream = HMAC-SHA256(key, nonce || counter) 每块 32 字节
//   cipher = plain XOR stream
//   tag    = HMAC-SHA256(key, "tag" || cipher)
// 存储格式: gdei.v1.<b64salt>.<b64nonce>.<b64cipher>.<b64tag>
//
// 说明：appPepper 是编译期常量，无法抵御逆向二进制的攻击者；真正的平台级保护
// 仍需 Linux Secret Service / Android Keystore。但相对裸 Base64，本实现消除了
// "直接可逆读取"这一 P0 根因，并带完整性校验（篡改会被 tag 校验拒绝），
// 且完全基于 Qt Core，全平台可编译。backend() 会如实标注为 encrypted fallback。
//
// Encrypted fallback: PBKDF2-derived key + HMAC keystream + encrypt-then-MAC.
// Removes the trivially-reversible Base64 exposure and adds tamper detection.
// A compiled-in pepper cannot resist binary reverse engineering; true platform
// keystore integration remains a future step, and backend() says so honestly.
#include "core/SecretStore.hpp"

#if !defined(Q_OS_WIN) && !defined(Q_OS_MACOS)

#include <QByteArray>
#include <QCryptographicHash>
#include <QMessageAuthenticationCode>
#include <QPasswordDigestor>
#include <QRandomGenerator>
#include <QGlobalStatic>
#include <QSettings>
#include <QString>
#include <QStringList>
#include <QtGlobal>

namespace gdei {
namespace {

constexpr const char* kSettingsOrg = "GDEI";
constexpr const char* kSettingsApp = "gdei-connect";
constexpr int kKdfIterations = 120000;
constexpr int kKeyLength = 32;
constexpr int kSaltLength = 16;
constexpr int kNonceLength = 16;
constexpr int kBlockSize = 32;

// 应用专属 pepper：与平台无关的固定熵，配合随机 salt 使用。
const QByteArray appPepper() {
    return QByteArrayLiteral("gdei-connect/secret-store/v1/pbkdf2-pepper");
}

QSettings openSettings() {
    return QSettings(QLatin1String(kSettingsOrg), QLatin1String(kSettingsApp));
}

QString secretKey(const QString& key) {
    return QStringLiteral("secret/") + key;
}

QByteArray randomBytes(int size) {
    QByteArray out;
    out.reserve(size);
    for (int i = 0; i < size; ++i) {
        out.append(static_cast<char>(QRandomGenerator::global()->bounded(256)));
    }
    return out;
}

QByteArray deriveKey(const QByteArray& salt) {
    // Qt6: QPasswordDigestor 属 QtNetwork，派生接口为 deriveKeyPbkdf2，
    // 算法枚举取自 QCryptographicHash（旧版 deriveKey/Algorithm 已移除）。
    return QPasswordDigestor::deriveKeyPbkdf2(QCryptographicHash::Sha256,
                                              appPepper(), salt, kKdfIterations, kKeyLength);
}

// HMAC-SHA256 作为 PRF 生成密钥流（CTR 风格，每块 32 字节）。
// HMAC-SHA256 as a PRF keystream (CTR-style, 32 bytes per block).
QByteArray xorWithKeystream(const QByteArray& input,
                            const QByteArray& key,
                            const QByteArray& nonce) {
    QByteArray out = input;
    QByteArray counter(4, Qt::Uninitialized);
    quint32 block = 0;
    for (int offset = 0; offset < input.size(); offset += kBlockSize, ++block) {
        counter[0] = static_cast<char>((block >> 24) & 0xFF);
        counter[1] = static_cast<char>((block >> 16) & 0xFF);
        counter[2] = static_cast<char>((block >> 8) & 0xFF);
        counter[3] = static_cast<char>(block & 0xFF);
        const QByteArray stream = QMessageAuthenticationCode::hash(
            nonce + counter, key, QCryptographicHash::Sha256);
        for (int i = 0; i < stream.size() && offset + i < input.size(); ++i) {
            out[offset + i] = out[offset + i] ^ stream[i];
        }
    }
    return out;
}

QByteArray authTag(const QByteArray& cipher, const QByteArray& key) {
    return QMessageAuthenticationCode::hash(
        QByteArrayLiteral("tag") + cipher, key, QCryptographicHash::Sha256);
}

// 加密；失败返回空字节数组。/ Encrypt; empty on failure.
QByteArray encryptSecret(const QString& secret) {
    const QByteArray salt = randomBytes(kSaltLength);
    const QByteArray nonce = randomBytes(kNonceLength);
    const QByteArray key = deriveKey(salt);
    if (key.size() != kKeyLength) return {};
    const QByteArray cipher = xorWithKeystream(secret.toUtf8(), key, nonce);
    const QByteArray tag = authTag(cipher, key);
    return QByteArrayLiteral("gdei.v1.") +
           salt.toBase64() + '.' + nonce.toBase64() + '.' +
           cipher.toBase64() + '.' + tag.toBase64();
}

// 解密；格式非法或 tag 校验失败返回空串。/ Decrypt; empty on invalid or bad tag.
QString decryptSecret(const QByteArray& stored) {
    if (!stored.startsWith("gdei.v1.")) {
        // 兼容旧版本裸 Base64，读取后由下次 write 自动升级为加密格式。
        // Migrate legacy plain-Base64 values; next write upgrades them.
        return QString::fromUtf8(QByteArray::fromBase64(stored));
    }
    const QByteArray body = stored.mid(8);
    const QList<QByteArray> parts = body.split('.');
    if (parts.size() != 4) return {};
    const QByteArray salt = QByteArray::fromBase64(parts[0]);
    const QByteArray nonce = QByteArray::fromBase64(parts[1]);
    const QByteArray cipher = QByteArray::fromBase64(parts[2]);
    const QByteArray tag = QByteArray::fromBase64(parts[3]);
    const QByteArray key = deriveKey(salt);
    if (key.size() != kKeyLength) return {};
    if (QMessageAuthenticationCode::hash(QByteArrayLiteral("tag") + cipher,
                                         key, QCryptographicHash::Sha256) != tag) {
        return {};  // 篡改 / 密钥不匹配：拒绝
    }
    return QString::fromUtf8(xorWithKeystream(cipher, key, nonce));
}

} // namespace

class FallbackSecretStore : public SecretStore {
public:
    bool write(const QString& key, const QString& secret) override {
        QSettings s = openSettings();
        if (secret.isEmpty()) {
            s.remove(secretKey(key));
            s.sync();
            return s.status() == QSettings::NoError;
        }
        const QByteArray encrypted = encryptSecret(secret);
        if (encrypted.isEmpty()) return false;
        s.setValue(secretKey(key), QString::fromLatin1(encrypted));
        s.sync();
        return s.status() == QSettings::NoError;
    }

    QString read(const QString& key) const override {
        QSettings s = openSettings();
        const QByteArray stored = s.value(secretKey(key)).toByteArray();
        if (stored.isEmpty()) return {};
        return decryptSecret(stored);
    }

    bool clear(const QString& key) override {
        QSettings s = openSettings();
        s.remove(secretKey(key));
        s.sync();
        return s.status() == QSettings::NoError;
    }

    QString backend() const override {
        return QStringLiteral("PBKDF2-HMAC-SHA256 encrypted QSettings fallback "
                              "(platform keystore pending)");
    }
};

SecretStore& fallbackSecretStore() {
    static FallbackSecretStore store;
    return store;
}

} // namespace gdei

#endif // !Q_OS_WIN && !Q_OS_MACOS
