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
class SessionController;
class LocalDeviceController;
class NetworkInfoController;
class EventBus;

class AppController : public QObject {
    Q_OBJECT
    Q_PROPERTY(QObject* config READ config CONSTANT)
    Q_PROPERTY(QObject* i18n READ i18n CONSTANT)
    Q_PROPERTY(QObject* auth READ auth CONSTANT)
    Q_PROPERTY(QObject* logs READ logs CONSTANT)
    Q_PROPERTY(QObject* session READ session CONSTANT)
    Q_PROPERTY(QObject* localDevices READ localDevices CONSTANT)
    Q_PROPERTY(QObject* network READ network CONSTANT)
    Q_PROPERTY(QObject* events READ events CONSTANT)
    Q_PROPERTY(QObject* eventModel READ eventModel CONSTANT)
    Q_PROPERTY(QObject* portalDevices READ portalDevices CONSTANT)

public:
    AppController(ConfigStore* config, Logger* logger, I18n* i18n,
                  AuthController* auth, LogController* logs,
                  SessionController* session, LocalDeviceController* localDevices,
                  NetworkInfoController* network, EventBus* events,
                  DeviceController* portalDevices, QObject* parent = nullptr);

    QObject* config() const;
    QObject* i18n() const;
    QObject* auth() const;
    QObject* logs() const;
    QObject* session() const;
    QObject* localDevices() const;
    QObject* network() const;
    QObject* events() const;
    QObject* eventModel() const;
    QObject* portalDevices() const;

private:
    ConfigStore* m_config;
    I18n* m_i18n;
    AuthController* m_auth;
    LogController* m_logs;
    SessionController* m_session;
    LocalDeviceController* m_localDevices;
    NetworkInfoController* m_network;
    EventBus* m_events;
    DeviceController* m_portalDevices;
};

} // namespace gdei

#endif // GDEI_APP_CONTROLLER_HPP
