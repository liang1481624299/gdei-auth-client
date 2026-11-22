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
    // 显式逐 QChar 取低 8 位构造字节，避免 toLatin1() 对非预期码位静默替换为 '?'
    // 导致生成错误口令。escapeStep 已把所有 >255 码位转成 ASCII \uXXXX。
    // Build bytes explicitly from the low byte of each QChar instead of relying on
    // toLatin1(), which silently substitutes '?' for unexpected code points.
    QByteArray bytes;
    bytes.reserve(escaped.size());
    for (const QChar ch : escaped) {
        const uint code = ch.unicode();
        if (code > 255) return QString();  // 防御：未转义的高码位，拒绝生成错误口令
        bytes.append(static_cast<char>(code & 0xFF));
    }
    return QString::fromLatin1(bytes.toBase64());
}

} // namespace SelfServiceCodec
} // namespace gdei
