// src/core/SelfServiceCodec.cpp
#include "core/SelfServiceCodec.hpp"

namespace gdei {
namespace SelfServiceCodec {

namespace {

// 对齐 JS escape() 的逐字符转义分支 / Per-char escape branch mirroring JS escape().
QString escapeStep(const QString& input) {
    QString out;
    out.reserve(input.size());
    for (const QChar ch : input) {
        const uint code = ch.unicode();
        if (code > 255) {
            // 中文等非 Latin-1 字符：%uXXXX -> 字面 \uXXXX（大写十六进制，4 位补零）。
            // Non-Latin-1: "%uXXXX" -> literal "\uXXXX" (uppercase hex, zero-padded).
            const QString hex = QString::number(code, 16).toUpper().rightJustified(4, QLatin1Char('0'));
            out += QStringLiteral("\\u") + hex;
        } else {
            // 其余字符原样保留，反斜杠双写 / Keep as-is; double the backslash.
            if (ch == QLatin1Char('\\')) {
                out += QStringLiteral("\\\\");
            } else {
                out += ch;
            }
        }
    }
    return out;
}

} // namespace

QString encodePassword(const QString& input) {
    const QString escaped = escapeStep(input);
    // 标准 Base64，按字符低 8 位取字节（与 bouncycastle_base64 一致）。
    // Standard Base64 over the low byte of each char, matching bouncycastle_base64.
    return QString::fromLatin1(escaped.toLatin1().toBase64());
}

} // namespace SelfServiceCodec
} // namespace gdei
