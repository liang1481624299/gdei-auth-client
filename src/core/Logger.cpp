// src/core/Logger.cpp
#include "core/Logger.hpp"
#include "core/Constants.hpp"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QStringConverter>
#include <QTextStream>

namespace gdei {

Logger::Logger(QObject* parent) : QObject(parent) {}

QVariantList Logger::entries() const {
    QVariantList out;
    for (const Entry& e : m_entries) {
        QVariantMap m;
        m["level"]     = static_cast<int>(e.level);
        m["levelText"] = levelText(e.level);
        m["message"]   = e.message;
        m["time"]      = timestamp(e.timestamp);
        out.append(m);
    }
    return out;
}

void Logger::debug(const QString& message)   { log(LogLevel::Debug, message); }
void Logger::info(const QString& message)    { log(LogLevel::Info, message); }
void Logger::warning(const QString& message) { log(LogLevel::Warning, message); }
void Logger::error(const QString& message)   { log(LogLevel::Error, message); }

void Logger::log(LogLevel level, const QString& message) {
    Entry e;
    e.level = level;
    e.message = sanitize(message);
    e.timestamp = QDateTime::currentMSecsSinceEpoch();
    append(e);

    // 同时写入持久日志文件 / also append to the persistent log file.
    const QString dir =
        QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dir);
    QFile file(QDir(dir).filePath("gdei-connect.log"));
    if (file.open(QIODevice::Append | QIODevice::Text)) {
        QTextStream ts(&file);
        ts.setEncoding(QStringConverter::Utf8);
        ts << timestamp(e.timestamp) << " [" << levelText(level) << "] "
           << e.message << "\n";
    }
}

QString Logger::exportLog(const QString& dirPath) {
    QString dir = dirPath.trimmed();
    if (dir.isEmpty()) {
        dir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    }
    QDir().mkpath(dir);
    const QString path = QDir(dir).filePath(
        QString("gdei-connect-%1.log")
            .arg(QDateTime::currentDateTime().toString("yyyyMMdd-HHmmss")));

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return QString();
    }
    QTextStream ts(&file);
    ts.setEncoding(QStringConverter::Utf8);
    for (const Entry& e : m_entries) {
        ts << timestamp(e.timestamp) << " [" << levelText(e.level) << "] "
           << e.message << "\n";
    }
    return path;
}

void Logger::append(Entry entry) {
    m_entries.append(entry);
    if (m_entries.size() > MAX_ENTRIES) {
        m_entries.removeFirst();
    }
    emit entriesChanged();
    emit logAdded(static_cast<int>(entry.level), entry.message);
}

QString Logger::levelText(LogLevel level) {
    switch (level) {
        case LogLevel::Debug:   return "DEBUG";
        case LogLevel::Info:    return "INFO";
        case LogLevel::Warning: return "WARN";
        case LogLevel::Error:   return "ERROR";
    }
    return "INFO";
}

QString Logger::sanitize(const QString& message) {
    QString out = message;

    // 键值对敏感字段：口令 / token / CSRF / 动态口令等，值统一掩码。
    // Mask secret key=value pairs: password, token, CSRF, dynamic password.
    QRegularExpression kv(
        "\\b(userPwd|password|passwd|pwd|token|csrf_token|freeOTP_secret_hidden|"
        "userDynamicPwddd|manualUrlEncryptKey|loginCodeInput)\\s*=\\s*([^\\s&;\"']+)",
        QRegularExpression::CaseInsensitiveOption);
    out.replace(kv, "\\1=***");

    // JSON 形式的敏感字段 "password": "..." / JSON key-value secrets.
    QRegularExpression jsonKv(
        "(\"(?:userPwd|password|passwd|pwd|token|csrf_token|loginCodeInput)\"\\s*:\\s*)"
        "\"[^\"]*\"",
        QRegularExpression::CaseInsensitiveOption);
    out.replace(jsonKv, "\\1\"***\"");

    // Cookie / Set-Cookie 整行掩码（含分号分隔的多段）/ Mask full Cookie headers,
    // including multiple semicolon-separated segments.
    QRegularExpression cookie(
        "\\b(Cookie|Set-Cookie)\\s*:\\s*[^\\r\\n]+",
        QRegularExpression::CaseInsensitiveOption);
    out.replace(cookie, "\\1: ***");

    // MAC 地址整体掩码 / Mask MAC addresses.
    QRegularExpression mac(
        "\\b(?:[0-9A-Fa-f]{2}:){5}[0-9A-Fa-f]{2}\\b");
    out.replace(mac, "**:**:**:**:**:**");

    // IPv4：保留前两段，后两段掩码 / Keep first two IPv4 octets, mask the rest.
    QRegularExpression ipv4(
        "\\b(\\d{1,3})\\.(\\d{1,3})\\.\\d{1,3}\\.\\d{1,3}\\b");
    out.replace(ipv4, "\\1.\\2.***.***");

    // Authorization: Bearer <token> / Bearer 令牌。
    QRegularExpression bearer(
        "\\bBearer\\s+[A-Za-z0-9._~+/=-]+",
        QRegularExpression::CaseInsensitiveOption);
    out.replace(bearer, "Bearer ***");

    // 裸 JWT（三段点分 Base64Url）/ Bare JWTs (three dot-separated Base64Url parts).
    QRegularExpression jwt(
        "\\b[A-Za-z0-9_-]{8,}\\.[A-Za-z0-9_-]{8,}\\.[A-Za-z0-9_-]{8,}\\b");
    out.replace(jwt, "***.***.***");

    // IPv6：整体掩码，避免地址泄露 / Mask IPv6 addresses.
    QRegularExpression ipv6(
        "\\b(?:[0-9A-Fa-f]{1,4}:){2,7}[0-9A-Fa-f]{0,4}\\b");
    out.replace(ipv6, "**:**:**:**");

    return out;
}

QString Logger::timestamp(qint64 ms) {
    return QDateTime::fromMSecsSinceEpoch(ms).toString("HH:mm:ss");
}

} // namespace gdei
