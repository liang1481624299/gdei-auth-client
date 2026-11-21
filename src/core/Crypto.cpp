// src/core/Crypto.cpp
#include "core/Crypto.hpp"

#include <QByteArray>

namespace gdei {

namespace {
const QByteArray kObfKey = QByteArray("gdei-connect-2026");

QByteArray xorWithKey(const QByteArray& in) {
    QByteArray out;
    out.reserve(in.size());
    for (int i = 0; i < in.size(); ++i) {
        out.append(static_cast<char>(
            static_cast<unsigned char>(in[i]) ^
            static_cast<unsigned char>(kObfKey[i % kObfKey.size()])));
    }
    return out;
}
} // namespace

QString Crypto::obfuscate(const QString& plainUtf8) {
    const QByteArray utf8 = plainUtf8.toUtf8();
    const QByteArray xored = xorWithKey(utf8);
    return QString::fromLatin1(xored.toBase64());
}

QString Crypto::deobfuscate(const QString& encoded) {
    if (encoded.isEmpty()) return QString();
    const QByteArray xored = QByteArray::fromBase64(encoded.toLatin1());
    const QByteArray utf8 = xorWithKey(xored);
    return QString::fromUtf8(utf8);
}

} // namespace gdei
