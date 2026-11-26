// src/core/Logger.hpp
// 分级日志 / Leveled logging: in-memory buffer + file export.
// 暴露给 QML 日志面板 / Exposed to the QML log panel.
#ifndef GDEI_LOGGER_HPP
#define GDEI_LOGGER_HPP

#include "core/Types.hpp"

#include <QObject>
#include <QString>
#include <QVariantList>
#include <QVector>

namespace gdei {

class Logger : public QObject {
    Q_OBJECT
    Q_PROPERTY(QVariantList entries READ entries NOTIFY entriesChanged)
public:
    explicit Logger(QObject* parent = nullptr);

    QVariantList entries() const;

    Q_INVOKABLE void debug(const QString& message);
    Q_INVOKABLE void info(const QString& message);
    Q_INVOKABLE void warning(const QString& message);
    Q_INVOKABLE void error(const QString& message);

    void log(LogLevel level, const QString& message);

    // 导出全部日志到指定目录，返回生成的文件路径。
    // Export all logs to a directory; returns the created file path.
    Q_INVOKABLE QString exportLog(const QString& dirPath);

signals:
    void entriesChanged();
    void logAdded(int level, QString message);

private:
    struct Entry {
        LogLevel level;
        QString  message;
        qint64   timestamp;
    };
    QVector<Entry> m_entries;
    static constexpr int MAX_ENTRIES = 500;

    // 持久日志大小上限：2 MiB × (1 当前 + 3 备份)。
    static constexpr qint64 MAX_FILE_BYTES = 2LL * 1024 * 1024;
    static constexpr int MAX_FILE_BACKUPS = 3;
    qint64 m_writtenBytes = -1;  // 首次写入前未知，读取实际文件大小

    void append(Entry entry);
    static void rotate(const QString& path);
    static QString levelText(LogLevel level);
    static QString timestamp(qint64 ms);
    static QString sanitize(const QString& message);
};

} // namespace gdei

#endif // GDEI_LOGGER_HPP
