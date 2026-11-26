// src/app/EventBridge.hpp
// 将现有 C++ 语义信号转换为 UI 事件副本，不修改业务逻辑。
#ifndef GDEI_EVENT_BRIDGE_HPP
#define GDEI_EVENT_BRIDGE_HPP

#include <QObject>
#include <QString>
#include <QVariantMap>

namespace gdei {

class AuthController;
class ConfigStore;
class DeviceController;
class EventBus;
class KeepAlive;
class LocalDeviceController;
class NetworkInfoController;
class PortalClient;
class SessionController;

class EventBridge : public QObject {
    Q_OBJECT
public:
    EventBridge(EventBus* events, AuthController* auth, SessionController* session,
                LocalDeviceController* localDevices, DeviceController* portalDevices,
                NetworkInfoController* network, ConfigStore* config,
                KeepAlive* keepAlive, PortalClient* portal, QObject* parent = nullptr);

private:
    void publish(int level, const QString& source, const QString& titleKey,
                 const QString& messageKey, const QString& reasonKey = QString(),
                 const QVariantMap& arguments = QVariantMap());
    void forward(int level, const QString& source, const QString& titleKey,
                 const QString& messageKey, const QString& reasonKey,
                 const QVariantMap& arguments);
    void onPortalDevicesFinished(bool ok, const QString& message);

    EventBus* m_events;
    AuthController* m_auth;
    SessionController* m_session;
    NetworkInfoController* m_network;
};

} // namespace gdei

#endif // GDEI_EVENT_BRIDGE_HPP
