// src/app/NetworkInfoController.hpp
// 本机活动网卡信息，不执行网络探测。
#ifndef GDEI_NETWORK_INFO_CONTROLLER_HPP
#define GDEI_NETWORK_INFO_CONTROLLER_HPP

#include <QObject>
#include <QVariantList>
#include <QVariantMap>

namespace gdei {

class ConfigStore;
class NicSelector;
class RouteBinder;

class NetworkInfoController : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString sourceIp READ sourceIp NOTIFY sourceIpChanged)
    Q_PROPERTY(QString targetIp READ targetIp NOTIFY targetIpChanged)
    Q_PROPERTY(QString netmask READ netmask NOTIFY netmaskChanged)
    Q_PROPERTY(QString localMac READ localMac NOTIFY localMacChanged)
    Q_PROPERTY(QString targetMac READ targetMac NOTIFY targetMacChanged)
    Q_PROPERTY(QString interfaceName READ interfaceName NOTIFY interfaceNameChanged)
    Q_PROPERTY(bool available READ available NOTIFY availableChanged)
    Q_PROPERTY(int subnetStatus READ subnetStatus NOTIFY subnetStatusChanged)
    Q_PROPERTY(QVariantList interfaces READ interfaces NOTIFY interfacesChanged)
    // 经门户可达性探测选出的校园网卡（C 层：仅识别标注，尚未改传输绑定）。
    // Campus NIC picked by portal reachability probe (layer C: detect & annotate only).
    Q_PROPERTY(QString campusSourceIp READ campusSourceIp NOTIFY campusChanged)
    Q_PROPERTY(QString campusInterfaceName READ campusInterfaceName NOTIFY campusChanged)
    Q_PROPERTY(bool campusReachable READ campusReachable NOTIFY campusChanged)
    // 选中的校园网卡与系统默认出站网卡不一致时为 true（门户可能走错网卡）。
    // True when the detected campus NIC differs from the default egress NIC.
    Q_PROPERTY(bool campusMismatch READ campusMismatch NOTIFY campusChanged)
    Q_PROPERTY(QVariantList campusNicList READ campusNicList NOTIFY campusChanged)
    Q_PROPERTY(QVariantMap campusSelected READ campusSelected NOTIFY campusChanged)
    // 层 B：主机路由强制状态（0 Idle / 1 Bound / 2 Degraded / 3 Unsupported）。
    // Layer B host-route status (0 Idle / 1 Bound / 2 Degraded / 3 Unsupported).
    Q_PROPERTY(int routeStatus READ routeStatus NOTIFY campusChanged)
    Q_PROPERTY(QString routeNote READ routeNote NOTIFY campusChanged)
    Q_PROPERTY(bool routeBound READ routeBound NOTIFY campusChanged)

public:
    explicit NetworkInfoController(ConfigStore* config, QObject* parent = nullptr);

    QString sourceIp() const;
    QString targetIp() const;
    QString netmask() const;
    QString localMac() const;
    QString targetMac() const;
    QString interfaceName() const;
    bool available() const;
    int subnetStatus() const;
    QVariantList interfaces() const;

    QString campusSourceIp() const;
    QString campusInterfaceName() const;
    bool campusReachable() const;
    bool campusMismatch() const;
    QVariantList campusNicList() const;
    QVariantMap campusSelected() const;

    int routeStatus() const;
    QString routeNote() const;
    bool routeBound() const;

    Q_INVOKABLE void refresh();

signals:
    void sourceIpChanged();
    void targetIpChanged();
    void netmaskChanged();
    void localMacChanged();
    void interfaceNameChanged();
    void availableChanged();
    void subnetStatusChanged();
    void targetMacChanged();
    void interfacesChanged();
    void campusChanged();

private:
    void updateTargetIp();

    ConfigStore* m_config;
    NicSelector* m_nicSelector = nullptr;
    RouteBinder* m_routeBinder = nullptr;
    QVariantList m_campusNicList;
    QVariantMap m_campusSelected;
    QString m_sourceIp;
    QString m_targetIp;
    QString m_netmask;
    QString m_localMac;
    QString m_targetMac;
    QString m_interfaceName;
    bool m_available = false;
    int m_subnetStatus = 0;
    QVariantList m_interfaces;
};

} // namespace gdei

#endif // GDEI_NETWORK_INFO_CONTROLLER_HPP
