#include "app/AppController.hpp"
#include "app/AccountController.hpp"
#include "app/AuthController.hpp"
#include "app/DeviceController.hpp"
#include "app/EventBus.hpp"
#include "app/LocalDeviceController.hpp"
#include "app/LogController.hpp"
#include "app/NetworkInfoController.hpp"
#include "app/DiagnosticsController.hpp"
#include "app/SessionController.hpp"
#include "core/CertificateStore.hpp"
#include "core/ConfigStore.hpp"
#include "core/I18n.hpp"
#include "core/Logger.hpp"

namespace gdei {

AppController::AppController(ConfigStore* config, Logger* /*logger*/, I18n* i18n,
                             AuthController* auth, LogController* logs,
                             SessionController* session, LocalDeviceController* localDevices,
                             NetworkInfoController* network, DiagnosticsController* diagnostics,
                             EventBus* events,
                             DeviceController* portalDevices, AccountController* account,
                             CertificateStore* certificates,
                             QObject* parent)
    : QObject(parent), m_config(config), m_i18n(i18n), m_auth(auth), m_logs(logs),
      m_session(session), m_localDevices(localDevices), m_network(network),
      m_diagnostics(diagnostics),
      m_events(events), m_portalDevices(portalDevices), m_account(account),
      m_certificates(certificates) {
    // 登出后清理自助门户会话，避免共享设备上的会话残留。
    connect(m_session, &SessionController::sessionCleared,
            m_portalDevices, &DeviceController::clearSession);
    connect(m_session, &SessionController::sessionCleared,
            m_account, &AccountController::clearSession);
}

QObject* AppController::config() const { return m_config; }
QObject* AppController::i18n() const { return m_i18n; }
QObject* AppController::auth() const { return m_auth; }
QObject* AppController::logs() const { return m_logs; }
QObject* AppController::session() const { return m_session; }
QObject* AppController::localDevices() const { return m_localDevices; }
QObject* AppController::network() const { return m_network; }
QObject* AppController::diagnostics() const { return m_diagnostics; }
QObject* AppController::events() const { return m_events; }
QObject* AppController::eventModel() const { return m_events->model(); }
QObject* AppController::portalDevices() const { return m_portalDevices; }
QObject* AppController::account() const { return m_account; }
QObject* AppController::certificates() const { return m_certificates; }

void AppController::applyTheme(int mode) {
    m_config->setThemeMode(mode);
    m_config->save();
}

void AppController::setLoginUsername(const QString& value) { m_config->setUsername(value); }
void AppController::setLoginPassword(const QString& value) { m_config->setPassword(value); }
void AppController::setRememberPassword(bool enabled) { m_config->setRememberPassword(enabled); }
void AppController::setAutoLogin(bool enabled) { m_config->setAutoLogin(enabled); }

bool AppController::saveAdvancedSettings(const QVariantMap& values) {
    if (values.contains(QStringLiteral("keepaliveMinSec")))
        m_config->setKeepaliveMinSec(values.value(QStringLiteral("keepaliveMinSec")).toInt());
    if (values.contains(QStringLiteral("keepaliveMaxSec")))
        m_config->setKeepaliveMaxSec(values.value(QStringLiteral("keepaliveMaxSec")).toInt());
    if (values.contains(QStringLiteral("offlineFailThreshold")))
        m_config->setOfflineFailThreshold(values.value(QStringLiteral("offlineFailThreshold")).toInt());
    if (values.contains(QStringLiteral("connectTimeoutMs")))
        m_config->setConnectTimeoutMs(values.value(QStringLiteral("connectTimeoutMs")).toInt());
    if (values.contains(QStringLiteral("probeTimeoutMs")))
        m_config->setProbeTimeoutMs(values.value(QStringLiteral("probeTimeoutMs")).toInt());
    if (values.contains(QStringLiteral("probeUrl")))
        m_config->setProbeUrl(values.value(QStringLiteral("probeUrl")).toString());
    if (values.contains(QStringLiteral("healthUrl")))
        m_config->setHealthUrl(values.value(QStringLiteral("healthUrl")).toString());
    if (values.contains(QStringLiteral("requestEncoding")))
        m_config->setRequestEncoding(values.value(QStringLiteral("requestEncoding")).toString());
    return m_config->save();
}

bool AppController::setSkipCertificateCheck(bool skip) {
    m_config->setAllowInsecureSsl(skip);
    return m_config->save();
}

void AppController::reportDiagnostic(int level, const QString& titleKey,
                                     const QString& detail) {
    if (level < 1 || level > 3) return;
    if (!titleKey.startsWith(QLatin1String("diagnostics."))) return;
    m_events->publish(level, QStringLiteral("diagnostics"), titleKey,
                      QStringLiteral("diagnostics.resultDetail"), QString(),
                      {{QStringLiteral("detail"), detail}});
}

} // namespace gdei
