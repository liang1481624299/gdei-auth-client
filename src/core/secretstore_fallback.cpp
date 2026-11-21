// src/core/secretstore_fallback.cpp
// 后备后端（Linux / Android）/ Fallback backend (Linux / Android).
// TODO(P1): Linux 接入 Secret Service (libsecret / D-Bus)，Android 接入 Keystore。
// 当前仅以 QSettings 保存 Base64，不提供平台级加密；后端名会明确标注，便于审计识别。
// TODO(P1): integrate Secret Service (libsecret/D-Bus) on Linux and Keystore on
// Android. The current fallback stores Base64 in QSettings without platform
// encryption; its backend name is explicit so it is identifiable in audits.
#include "core/SecretStore.hpp"

#if !defined(Q_OS_WIN) && !defined(Q_OS_MACOS)

#include <QByteArray>
#include <QSettings>
#include <QString>

namespace gdei {

class FallbackSecretStore : public SecretStore {
public:
    bool write(const QString& key, const QString& secret) override {
        QSettings s(QLatin1String("GDEI"), QLatin1String("gdei-connect"));
        if (secret.isEmpty()) {
            s.remove(QStringLiteral("secret/") + key);
            return true;
        }
        s.setValue(QStringLiteral("secret/") + key,
                   QString::fromLatin1(secret.toUtf8().toBase64()));
        return true;
    }

    QString read(const QString& key) const override {
        QSettings s(QLatin1String("GDEI"), QLatin1String("gdei-connect"));
        const QString b64 = s.value(QStringLiteral("secret/") + key, QString()).toString();
        if (b64.isEmpty()) return {};
        return QString::fromUtf8(QByteArray::fromBase64(b64.toLatin1()));
    }

    bool clear(const QString& key) override {
        QSettings s(QLatin1String("GDEI"), QLatin1String("gdei-connect"));
        s.remove(QStringLiteral("secret/") + key);
        return true;
    }

    QString backend() const override {
        return QStringLiteral("QSettings-fallback (Secret Service / Keystore pending)");
    }
};

SecretStore& fallbackSecretStore() {
    static FallbackSecretStore store;
    return store;
}

} // namespace gdei

#endif // !Q_OS_WIN && !Q_OS_MACOS
