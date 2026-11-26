// src/core/BodyDecoder.cpp
#include "core/BodyDecoder.hpp"

#include <QStringDecoder>
#include <QTextCodec>

namespace gdei {
namespace BodyDecoder {

QString fromLegacy(const QByteArray& raw) {
    if (raw.isEmpty()) return QString();

    // UTF-8 BOM：明确声明优先于探测。
    if (raw.startsWith("\xEF\xBB\xBF")) {
        return QString::fromUtf8(raw.constData() + 3, raw.size() - 3);
    }

    // 纯 ASCII 与合法 UTF-8 直接返回；非法序列才进入 GB18030 分支。
    QStringDecoder utf8(QStringDecoder::Utf8, QStringDecoder::Flag::Stateless);
    const QString text = utf8.decode(raw);
    if (!utf8.hasError()) return text;

    if (QTextCodec* codec = QTextCodec::codecForName("GB18030")) {
        return codec->toUnicode(raw);
    }
    if (QTextCodec* codec = QTextCodec::codecForName("GBK")) {
        return codec->toUnicode(raw);
    }
    return QString::fromUtf8(raw);
}

} // namespace BodyDecoder
} // namespace gdei
