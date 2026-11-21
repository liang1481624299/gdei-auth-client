// src/app/LogController.cpp
#include "app/LogController.hpp"
#include "core/Logger.hpp"

namespace gdei {

LogController::LogController(Logger* logger, QObject* parent)
    : QObject(parent), m_log(logger) {
    connect(m_log, &Logger::entriesChanged, this, &LogController::entriesChanged);
}

QVariantList LogController::entries() const { return m_log->entries(); }

QString LogController::exportLog(const QString& dirPath) {
    return m_log->exportLog(dirPath);
}

} // namespace gdei
