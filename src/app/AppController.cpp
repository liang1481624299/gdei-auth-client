// src/app/AppController.cpp
#include "app/AppController.hpp"
#include "app/AuthController.hpp"
#include "app/DeviceController.hpp"
#include "app/LogController.hpp"
#include "core/ConfigStore.hpp"
#include "core/I18n.hpp"
#include "core/Logger.hpp"

namespace gdei {

AppController::AppController(ConfigStore* config, Logger* /*logger*/, I18n* i18n,
                             AuthController* auth, LogController* logs,
                             DeviceController* devices, QObject* parent)
    : QObject(parent), m_config(config), m_i18n(i18n), m_auth(auth),
      m_logs(logs), m_devices(devices) {}

QObject* AppController::config() const { return m_config; }
QObject* AppController::i18n() const { return m_i18n; }
QObject* AppController::auth() const { return m_auth; }
QObject* AppController::logs() const { return m_logs; }
QObject* AppController::devices() const { return m_devices; }

} // namespace gdei
