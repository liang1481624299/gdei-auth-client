// src/app/AppController.hpp
// 聚合控制器 / Root aggregator: hands core objects to the QML root as `app`.
#ifndef GDEI_APP_CONTROLLER_HPP
#define GDEI_APP_CONTROLLER_HPP

#include <QObject>

namespace gdei {

class ConfigStore;
class Logger;
class I18n;
class AuthController;
class LogController;
class DeviceController;

class AppController : public QObject {
    Q_OBJECT
    Q_PROPERTY(QObject* config READ config CONSTANT)
    Q_PROPERTY(QObject* i18n READ i18n CONSTANT)
    Q_PROPERTY(QObject* auth READ auth CONSTANT)
    Q_PROPERTY(QObject* logs READ logs CONSTANT)
    Q_PROPERTY(QObject* devices READ devices CONSTANT)

public:
    AppController(ConfigStore* config, Logger* logger, I18n* i18n,
                  AuthController* auth, LogController* logs,
                  DeviceController* devices, QObject* parent = nullptr);

    QObject* config() const;
    QObject* i18n() const;
    QObject* auth() const;
    QObject* logs() const;
    QObject* devices() const;

private:
    ConfigStore* m_config;
    I18n* m_i18n;
    AuthController* m_auth;
    LogController* m_logs;
    DeviceController* m_devices;
};

} // namespace gdei

#endif // GDEI_APP_CONTROLLER_HPP
