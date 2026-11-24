// src/core/FormEncoding.cpp
#include "core/FormEncoding.hpp"

#include <QStringDecoder>
#include <QStringList>
#include <QTextCodec>

namespace gdei {
namespace FormEncoding {
namespace {

QString normalizeEncoding(const QString& value) {
    QString normalized = value.trimmed().toUpper();
    normalized.remove(QLatin1Char('-'));
    if (normalized == "AUTO" || normalized == "AUTODETECT") return QStringLiteral("AUTO");
    if (normalized == "ASCII") return QStringLiteral("ASCII");
    if (normalized == "GBK") return QStringLiteral("GBK");
    return QStringLiteral("UTF-8");
}

int hexValue(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    return -1;
}

QByteArray percentEncode(const QByteArray& input) {
    static constexpr char hex[] = "0123456789ABCDEF";
    QByteArray output;
    output.reserve(input.size() * 3);
    for (const unsigned char c : input) {
        const bool unreserved = (c >= 'A' && c <= 'Z') ||
                                (c >= 'a' && c <= 'z') ||
                                (c >= '0' && c <= '9') ||
                                c == '-' || c == '.' || c == '_' || c == '~';
        if (unreserved) {
            output.append(static_cast<char>(c));
        } else {
            output.append('%');
            output.append(hex[c >> 4]);
            output.append(hex[c & 0x0f]);
        }
    }
    return output;
}

bool decodeComponent(const QString& source, QString& decodedText) {
    const QByteArray input = source.toUtf8();
    QByteArray decoded;
    decoded.reserve(input.size());

    for (qsizetype i = 0; i < input.size();) {
        if (input[i] != '%') {
            decoded.append(input[i]);
            ++i;
            continue;
        }
        if (i + 2 >= input.size()) return false;
        const int high = hexValue(input[i + 1]);
        const int low = hexValue(input[i + 2]);
        if (high < 0 || low < 0) return false;
        decoded.append(static_cast<char>((high << 4) | low));
        i += 3;
    }

    QStringDecoder decoder(QStringDecoder::Utf8);
    decodedText = decoder.decode(decoded);
    return !decoder.hasError();
}

Result failed(Result result, Error error, const QString& key) {
    result.error = error;
    result.errorKey = key;
    return result;
}

} // namespace

Result encode(const QString& body, const QString& encoding) {
    Result result;
    result.charset = normalizeEncoding(encoding);
    // AUTO 尚未在登录流程回填探测结果前，按 UTF-8 兜底序列化表单。
    // AUTO falls back to UTF-8 until the detected charset is backfilled by the login flow.
    if (result.charset == "UTF-8" || result.charset == "AUTO") {
        result.body = body.toUtf8();
        return result;
    }

    QTextCodec* gbkCodec = nullptr;
    if (result.charset == "GBK") {
        gbkCodec = QTextCodec::codecForName("GBK");
        if (!gbkCodec) {
            return failed(result, Error::Unmappable, QStringLiteral("encoding.unmappable"));
        }
    }

    const QStringList fields = body.split(QLatin1Char('&'), Qt::KeepEmptyParts);
    QByteArray output;
    for (qsizetype i = 0; i < fields.size(); ++i) {
        const QString& field = fields[i];
        const qsizetype separator = field.indexOf(QLatin1Char('='));
        if (separator < 0) {
            return failed(result, Error::InvalidForm, QStringLiteral("encoding.invalidForm"));
        }

        const QString sources[] = {field.left(separator), field.mid(separator + 1)};
        QByteArray converted[2];
        for (int part = 0; part < 2; ++part) {
            QString decoded;
            if (!decodeComponent(sources[part], decoded)) {
                return failed(result, Error::InvalidForm, QStringLiteral("encoding.invalidForm"));
            }

            if (result.charset == "ASCII") {
                for (const QChar c : decoded) {
                    if (c.unicode() > 0x7f) {
                        return failed(result, Error::NonAscii, QStringLiteral("encoding.nonAscii"));
                    }
                }
                converted[part] = decoded.toLatin1();
            } else {
                QTextCodec::ConverterState state;
                converted[part] = gbkCodec->fromUnicode(decoded.constData(), decoded.size(), &state);
                if (state.invalidChars > 0) {
                    return failed(result, Error::Unmappable, QStringLiteral("encoding.unmappable"));
                }
            }
        }

        if (i > 0) output.append('&');
        output.append(percentEncode(converted[0]));
        output.append('=');
        output.append(percentEncode(converted[1]));
    }

    result.body = output;
    return result;
}

} // namespace FormEncoding
} // namespace gdei
