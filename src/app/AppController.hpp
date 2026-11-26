// src/app/AppController.hpp
// 聚合控制器 / Root aggregator: hands core objects to the QML root as `app`.
#ifndef GDEI_APP_CONTROLLER_HPP
#define GDEI_APP_CONTROLLER_HPP

#include <QObject>
#include <QString>
#include <QVariantMap>

namespace gdei {

class ConfigStore;
class Logger;
class I18n;
class AuthController;
class LogController;
class DeviceController;
class AccountController;
class SessionController;
class LocalDeviceController;
class NetworkInfoController;
class DiagnosticsController;
class EventBus;
class CertificateStore;

class AppController : public QObject {
    Q_OBJECT
    Q_PROPERTY(QObject* config READ config CONSTANT)
    Q_PROPERTY(QObject* i18n READ i18n CONSTANT)
    Q_PROPERTY(QObject* auth READ auth CONSTANT)
    Q_PROPERTY(QObject* logs READ logs CONSTANT)
    Q_PROPERTY(QObject* session READ session CONSTANT)
    Q_PROPERTY(QObject* localDevices READ localDevices CONSTANT)
    Q_PROPERTY(QObject* network READ network CONSTANT)
    Q_PROPERTY(QObject* diagnostics READ diagnostics CONSTANT)
    Q_PROPERTY(QObject* events READ events CONSTANT)
    Q_PROPERTY(QObject* eventModel READ eventModel CONSTANT)
    Q_PROPERTY(QObject* portalDevices READ portalDevices CONSTANT)
    Q_PROPERTY(QObject* account READ account CONSTANT)
    Q_PROPERTY(QObject* certificates READ certificates CONSTANT)

public:
    AppController(ConfigStore* config, Logger* logger, I18n* i18n,
                  AuthController* auth, LogController* logs,
                  SessionController* session, LocalDeviceController* localDevices,
                  NetworkInfoController* network, DiagnosticsController* diagnostics,
                  EventBus* events,
                  DeviceController* portalDevices, AccountController* account,
                  CertificateStore* certificates, QObject* parent = nullptr);

    QObject* config() const;
    QObject* i18n() const;
    QObject* auth() const;
    QObject* logs() const;
    QObject* session() const;
    QObject* localDevices() const;
    QObject* network() const;
    QObject* diagnostics() const;
    QObject* events() const;
    QObject* eventModel() const;
    QObject* portalDevices() const;
    QObject* account() const;
    QObject* certificates() const;

    // ── 设置门面 / Settings facade ──
    // QML 只调用门面方法；校验、内存状态与持久化统一在 C++ 完成。
    // QML calls these methods only; validation and persistence stay in C++.
    Q_INVOKABLE void applyTheme(int mode);
    Q_INVOKABLE void setLoginUsername(const QString& value);
    Q_INVOKABLE void setLoginPassword(const QString& value);
    Q_INVOKABLE void setRememberPassword(bool enabled);
    Q_INVOKABLE void setAutoLogin(bool enabled);
    Q_INVOKABLE bool saveAdvancedSettings(const QVariantMap& values);
    Q_INVOKABLE bool setSkipCertificateCheck(bool skip);

    // 诊断页结果进入全局事件队列。仅接受 diagnostics.* 标题键，避免 QML 伪造
    // 网络/保活等业务事件。
    // Diagnostic results only: accepts diagnostics.* keys so QML cannot fabricate
    // network/keep-alive business events.
    Q_INVOKABLE void reportDiagnostic(int level, const QString& titleKey,
                                      const QString& detail);

private:
    ConfigStore* m_config;
    I18n* m_i18n;
    AuthController* m_auth;
    LogController* m_logs;
    SessionController* m_session;
    LocalDeviceController* m_localDevices;
    NetworkInfoController* m_network;
    DiagnosticsController* m_diagnostics;
    EventBus* m_events;
    DeviceController* m_portalDevices;
    AccountController* m_account;
    CertificateStore* m_certificates;
};

} // namespace gdei

#endif // GDEI_APP_CONTROLLER_HPP
