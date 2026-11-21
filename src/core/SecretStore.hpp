// src/core/SecretStore.hpp
// 平台安全存储抽象 / Platform-backed secret storage abstraction.
// 口令等敏感凭据经平台安全能力落盘：Windows DPAPI、macOS Keychain；
// Linux / Android 在 Secret Service / Keystore 后端落地前使用明确标注的后备实现。
// Secrets are persisted through platform secure storage: DPAPI on Windows,
// Keychain on macOS; Linux/Android use a clearly marked fallback until the
// Secret Service / Keystore backend lands.
#ifndef GDEI_SECRET_STORE_HPP
#define GDEI_SECRET_STORE_HPP

#include <QString>

namespace gdei {

class SecretStore {
public:
    virtual ~SecretStore() = default;

    // 写入（或覆盖）指定密钥项；空串视为清除。
    // Write (or overwrite) the secret for `key`; an empty secret clears it.
    virtual bool write(const QString& key, const QString& secret) = 0;

    // 读取密钥项；不存在或失败时返回空串。
    // Read the secret; returns an empty string when absent or on failure.
    virtual QString read(const QString& key) const = 0;

    // 清除指定密钥项 / Remove the secret for `key`.
    virtual bool clear(const QString& key) = 0;

    // 后端名称，用于日志审计与排障 / Backend name for audit logs.
    virtual QString backend() const = 0;

    // 进程内单例 / Process-wide singleton.
    static SecretStore& instance();
};

} // namespace gdei

#endif // GDEI_SECRET_STORE_HPP
