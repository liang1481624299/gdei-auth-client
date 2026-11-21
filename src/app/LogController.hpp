// src/app/LogController.hpp
// 日志控制器 / Log controller: exposes Logger to QML.
#ifndef GDEI_LOG_CONTROLLER_HPP
#define GDEI_LOG_CONTROLLER_HPP

#include <QObject>
#include <QString>
#include <QVariantList>

namespace gdei {

class Logger;

class LogController : public QObject {
    Q_OBJECT
    Q_PROPERTY(QVariantList entries READ entries NOTIFY entriesChanged)
public:
    explicit LogController(Logger* logger, QObject* parent = nullptr);

    QVariantList entries() const;
    Q_INVOKABLE QString exportLog(const QString& dirPath);

signals:
    void entriesChanged();

private:
    Logger* m_log;
};

} // namespace gdei

#endif // GDEI_LOG_CONTROLLER_HPP
