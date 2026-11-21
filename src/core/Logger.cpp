// src/core/Logger.cpp
#include "core/Logger.hpp"
#include "core/Constants.hpp"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QStandardPaths>
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
    e.message = message;
    e.timestamp = QDateTime::currentMSecsSinceEpoch();
    append(e);

    // 同时写入持久日志文件 / also append to the persistent log file.
    const QString dir =
        QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dir);
    QFile file(QDir(dir).filePath("gdei-connect.log"));
    if (file.open(QIODevice::Append | QIODevice::Text)) {
        QTextStream ts(&file);
        ts << timestamp(e.timestamp) << " [" << levelText(level) << "] "
           << message << "\n";
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

QString Logger::timestamp(qint64 ms) {
    return QDateTime::fromMSecsSinceEpoch(ms).toString("HH:mm:ss");
}

} // namespace gdei
