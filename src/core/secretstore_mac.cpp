// src/core/secretstore_mac.cpp
// macOS Keychain 后端 / macOS Keychain backend.
// 凭据存入 Generic Password 项（服务名 gdei-connect，账号为密钥名），
// 由 macOS Keychain 负责加密与访问控制，不使用硬编码固定密钥。
// Secrets are stored as Generic Password items (service gdei-connect,
// account = key); the Keychain handles encryption and access control.
#include "core/SecretStore.hpp"

#ifdef Q_OS_MACOS

#include <Security/Security.h>

#include <QByteArray>
#include <QString>

namespace gdei {

namespace {
constexpr const char* kService = "gdei-connect";

CFMutableDictionaryRef baseQuery(const QString& account) {
    CFMutableDictionaryRef query = CFDictionaryCreateMutable(
        nullptr, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);

    CFStringRef service = CFStringCreateWithCString(
        nullptr, kService, kCFStringEncodingUTF8);
    CFStringRef acc = CFStringCreateWithCString(
        nullptr, account.toUtf8().constData(), kCFStringEncodingUTF8);

    CFDictionaryAddValue(query, kSecClass, kSecClassGenericPassword);
    CFDictionaryAddValue(query, kSecAttrService, service);
    CFDictionaryAddValue(query, kSecAttrAccount, acc);

    CFRelease(service);
    CFRelease(acc);
    return query;
}
} // namespace

class MacSecretStore : public SecretStore {
public:
    bool write(const QString& key, const QString& secret) override {
        if (secret.isEmpty()) {
            clear(key);
            return true;
        }

        const QByteArray utf8 = secret.toUtf8();
        CFDataRef data = CFDataCreate(
            nullptr, reinterpret_cast<const UInt8*>(utf8.constData()), utf8.size());

        CFMutableDictionaryRef query = baseQuery(key);
        CFDictionaryAddValue(query, kSecValueData, data);

        OSStatus status = SecItemAdd(query, nullptr);
        if (status == errSecDuplicateItem) {
            CFMutableDictionaryRef attrs = CFDictionaryCreateMutable(
                nullptr, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
            CFDictionaryAddValue(attrs, kSecValueData, data);
            status = SecItemUpdate(query, attrs);
            CFRelease(attrs);
        }

        CFRelease(data);
        CFRelease(query);
        return status == errSecSuccess;
    }

    QString read(const QString& key) const override {
        CFMutableDictionaryRef query = baseQuery(key);
        CFDictionaryAddValue(query, kSecReturnData, kCFBooleanTrue);

        CFDataRef data = nullptr;
        OSStatus status = SecItemCopyMatching(query, reinterpret_cast<CFTypeRef*>(&data));
        CFRelease(query);
        if (status != errSecSuccess || !data) return {};

        QByteArray plain(reinterpret_cast<const char*>(CFDataGetBytePtr(data)),
                         CFDataGetLength(data));
        CFRelease(data);
        return QString::fromUtf8(plain);
    }

    bool clear(const QString& key) override {
        CFMutableDictionaryRef query = baseQuery(key);
        SecItemDelete(query);
        CFRelease(query);
        return true;
    }

    QString backend() const override { return QStringLiteral("Keychain"); }
};

SecretStore& macSecretStore() {
    static MacSecretStore store;
    return store;
}

} // namespace gdei

#endif // Q_OS_MACOS
