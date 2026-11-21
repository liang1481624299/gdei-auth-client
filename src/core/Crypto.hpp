// src/core/Crypto.hpp
// 本地口令混淆 / Local password obfuscation.
// 注意：这是可逆混淆，避免明文落盘；后续可替换为系统钥匙串
// (Windows Credential Manager / macOS Keychain / Android Keystore)。
// Reversible obfuscation only; swap for platform keychains later.
#ifndef GDEI_CRYPTO_HPP
#define GDEI_CRYPTO_HPP

#include <QString>

namespace gdei {

class Crypto {
public:
    // 明文(UTF-8) -> 混淆后的 Base64 字符串。
    static QString obfuscate(const QString& plainUtf8);
    // 还原明文 / restore plaintext.
    static QString deobfuscate(const QString& encoded);
};

} // namespace gdei

#endif // GDEI_CRYPTO_HPP
