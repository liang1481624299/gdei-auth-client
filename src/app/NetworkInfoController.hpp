// src/app/NetworkInfoController.hpp
// 本机活动网卡信息，不执行网络探测。
#ifndef GDEI_NETWORK_INFO_CONTROLLER_HPP
#define GDEI_NETWORK_INFO_CONTROLLER_HPP

#include <QObject>
#include <QVariantList>

namespace gdei {

class ConfigStore;

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

private:
    void updateTargetIp();

    ConfigStore* m_config;
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
