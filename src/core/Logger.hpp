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

    void append(Entry entry);
    static QString levelText(LogLevel level);
    static QString timestamp(qint64 ms);
};

} // namespace gdei

#endif // GDEI_LOGGER_HPP
