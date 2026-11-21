// src/core/secretstore_win.cpp
// Windows DPAPI 后端 / Windows DPAPI backend.
// 凭据经 CryptProtectData 按当前用户 + 机器加密，密文以 Base64 存于 QSettings；
// 不使用任何硬编码固定密钥。
// Secrets are encrypted with CryptProtectData (current user + machine) and
// stored as Base64 in QSettings; no hardcoded fixed key is used.
#include "core/SecretStore.hpp"

#ifdef Q_OS_WIN

#include <windows.h>
#include <dpapi.h>

#include <QByteArray>
#include <QSettings>
#include <QString>

namespace gdei {

namespace {
constexpr const char* kSettingsOrg = "GDEI";
constexpr const char* kSettingsApp = "gdei-connect";

QSettings openSettings() {
    return QSettings(QLatin1String(kSettingsOrg), QLatin1String(kSettingsApp));
}

QString secretKey(const QString& key) {
    return QStringLiteral("secret/") + key;
}

QByteArray protectBlob(const QByteArray& plain) {
    DATA_BLOB inBlob;
    inBlob.pbData = const_cast<BYTE*>(reinterpret_cast<const BYTE*>(plain.constData()));
    inBlob.cbData = static_cast<DWORD>(plain.size());

    DATA_BLOB outBlob{0, nullptr};
    if (!CryptProtectData(&inBlob, nullptr, nullptr, nullptr, nullptr, 0, &outBlob)) {
        return {};
    }
    QByteArray result(reinterpret_cast<const char*>(outBlob.pbData), outBlob.cbData);
    LocalFree(outBlob.pbData);
    return result;
}

QByteArray unprotectBlob(const QByteArray& blob) {
    DATA_BLOB inBlob;
    inBlob.pbData = const_cast<BYTE*>(reinterpret_cast<const BYTE*>(blob.constData()));
    inBlob.cbData = static_cast<DWORD>(blob.size());

    DATA_BLOB outBlob{0, nullptr};
    LPWSTR description = nullptr;
    if (!CryptUnprotectData(&inBlob, &description, nullptr, nullptr, nullptr, 0, &outBlob)) {
        return {};
    }
    QByteArray result(reinterpret_cast<const char*>(outBlob.pbData), outBlob.cbData);
    LocalFree(outBlob.pbData);
    if (description) LocalFree(description);
    return result;
}
} // namespace

class WinSecretStore : public SecretStore {
public:
    bool write(const QString& key, const QString& secret) override {
        QSettings s = openSettings();
        if (secret.isEmpty()) {
            s.remove(secretKey(key));
            return true;
        }
        const QByteArray blob = protectBlob(secret.toUtf8());
        if (blob.isEmpty()) return false;
        s.setValue(secretKey(key), QString::fromLatin1(blob.toBase64()));
        return true;
    }

    QString read(const QString& key) const override {
        QSettings s = openSettings();
        const QString b64 = s.value(secretKey(key), QString()).toString();
        if (b64.isEmpty()) return {};
        const QByteArray plain = unprotectBlob(QByteArray::fromBase64(b64.toLatin1()));
        return QString::fromUtf8(plain);
    }

    bool clear(const QString& key) override {
        QSettings s = openSettings();
        s.remove(secretKey(key));
        return true;
    }

    QString backend() const override { return QStringLiteral("DPAPI"); }
};

SecretStore& winSecretStore() {
    static WinSecretStore store;
    return store;
}

} // namespace gdei

#endif // Q_OS_WIN
