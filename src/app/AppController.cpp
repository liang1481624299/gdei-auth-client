#include "app/AppController.hpp"
#include "app/AuthController.hpp"
#include "app/DeviceController.hpp"
#include "app/EventBus.hpp"
#include "app/LocalDeviceController.hpp"
#include "app/LogController.hpp"
#include "app/NetworkInfoController.hpp"
#include "app/SessionController.hpp"
#include "core/ConfigStore.hpp"
#include "core/I18n.hpp"
#include "core/Logger.hpp"

namespace gdei {

AppController::AppController(ConfigStore* config, Logger* /*logger*/, I18n* i18n,
                             AuthController* auth, LogController* logs,
                             SessionController* session, LocalDeviceController* localDevices,
                             NetworkInfoController* network, EventBus* events,
                             DeviceController* portalDevices, QObject* parent)
    : QObject(parent), m_config(config), m_i18n(i18n), m_auth(auth), m_logs(logs),
      m_session(session), m_localDevices(localDevices), m_network(network),
      m_events(events), m_portalDevices(portalDevices) {
    // 登出后清理自助门户会话，避免共享设备上的会话残留。
    connect(m_session, &SessionController::sessionCleared,
            m_portalDevices, &DeviceController::clearSession);
}

QObject* AppController::config() const { return m_config; }
QObject* AppController::i18n() const { return m_i18n; }
QObject* AppController::auth() const { return m_auth; }
QObject* AppController::logs() const { return m_logs; }
QObject* AppController::session() const { return m_session; }
QObject* AppController::localDevices() const { return m_localDevices; }
QObject* AppController::network() const { return m_network; }
QObject* AppController::events() const { return m_events; }
QObject* AppController::eventModel() const { return m_events->model(); }
QObject* AppController::portalDevices() const { return m_portalDevices; }

} // namespace gdei
