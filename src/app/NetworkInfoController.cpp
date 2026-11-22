// src/app/NetworkInfoController.cpp
#include "app/NetworkInfoController.hpp"
#include "core/ConfigStore.hpp"

#include <QAbstractSocket>
#include <QHostAddress>
#include <QNetworkAddressEntry>
#include <QNetworkInterface>
#include <QVariantMap>

namespace gdei {
namespace {

// 虚拟 / 隧道 / 桥接接口关键词；主网卡选择时优先排除。
// Keywords for virtual / tunnel / bridge adapters; deprioritized when picking the primary NIC.
const QStringList kVirtualKeywords = {
    "vethernet", "hyper-v", "vmware", "virtualbox", "vbox", "wsl", "tailscale",
    "tun", "tap", "docker", "veth", "vmxnet", "virtual", "virtualbox", "hyperv",
    "warp", "zerotier", "openvpn", "wireguard"
};

bool isVirtualAdapter(const QNetworkInterface& iface) {
    const QString name = iface.name().toLower();
    const QString human = iface.humanReadableName().toLower();
    for (const QString& kw : kVirtualKeywords) {
        if (name.contains(kw) || human.contains(kw)) return true;
    }
    return false;
}

bool isLinkLocal(const QHostAddress& ip) {
    return ip.isInSubnet(QHostAddress("169.254.0.0"), 16);
}

int calculateSubnetStatus(const QString& sourceIp, const QString& targetIp,
                          const QString& netmask) {
    QHostAddress source;
    QHostAddress target;
    QHostAddress mask;
    if (!source.setAddress(sourceIp) || !target.setAddress(targetIp) || !mask.setAddress(netmask)) {
        return 0;
    }
    if (source.protocol() != QAbstractSocket::IPv4Protocol ||
        target.protocol() != QAbstractSocket::IPv4Protocol ||
        mask.protocol() != QAbstractSocket::IPv4Protocol) {
        return 0;
    }
    const quint32 maskValue = mask.toIPv4Address();
    return (source.toIPv4Address() & maskValue) == (target.toIPv4Address() & maskValue) ? 1 : 2;
}

} // namespace

NetworkInfoController::NetworkInfoController(ConfigStore* config, QObject* parent)
    : QObject(parent), m_config(config) {
    connect(m_config, &ConfigStore::ipChanged, this, &NetworkInfoController::updateTargetIp);
    refresh();
}

QString NetworkInfoController::sourceIp() const { return m_sourceIp; }
QString NetworkInfoController::targetIp() const { return m_targetIp; }
QString NetworkInfoController::netmask() const { return m_netmask; }
QString NetworkInfoController::localMac() const { return m_localMac; }
QString NetworkInfoController::targetMac() const { return QString(); }
QString NetworkInfoController::interfaceName() const { return m_interfaceName; }
bool NetworkInfoController::available() const { return m_available; }
int NetworkInfoController::subnetStatus() const { return m_subnetStatus; }
QVariantList NetworkInfoController::interfaces() const { return m_interfaces; }

void NetworkInfoController::refresh() {
    QString sourceIp;
    QString netmask;
    QString localMac;
    QString interfaceName;
    QVariantList interfaces;
    bool havePrimary = false;
    int bestScore = -1;

    const auto allInterfaces = QNetworkInterface::allInterfaces();
    for (const QNetworkInterface& networkInterface : allInterfaces) {
        const QNetworkInterface::InterfaceFlags flags = networkInterface.flags();
        if (!(flags & QNetworkInterface::IsUp) ||
            !(flags & QNetworkInterface::IsRunning) ||
            (flags & QNetworkInterface::IsLoopBack)) {
            continue;
        }
        const bool virtualAdapter = isVirtualAdapter(networkInterface);
        const auto entries = networkInterface.addressEntries();
        for (const QNetworkAddressEntry& entry : entries) {
            if (entry.ip().protocol() != QAbstractSocket::IPv4Protocol || entry.ip().isLoopback()) {
                continue;
            }
            const QString entryIp = entry.ip().toString();
            const QString entryNetmask = entry.netmask().toString();
            const QString entryMac = networkInterface.hardwareAddress();
            interfaces.append(QVariantMap{
                {QStringLiteral("name"), networkInterface.name()},
                {QStringLiteral("ip"), entryIp},
                {QStringLiteral("netmask"), entryNetmask},
                {QStringLiteral("mac"), entryMac}
            });

            // 主网卡打分：非虚拟活动网卡优先，其次虚拟 / 隧道；link-local（如 Tailscale 169.254）最低。
            // Score the primary NIC: plain non-virtual adapter first, then virtual/tunnel;
            // link-local (e.g. Tailscale 169.254) ranks last.
            int score;
            if (isLinkLocal(entry.ip())) {
                score = 0;
            } else if (!virtualAdapter) {
                score = 2;
            } else {
                score = 1;
            }
            if (!entryMac.isEmpty()) ++score;

            if (score > bestScore) {
                bestScore = score;
                sourceIp = entryIp;
                netmask = entryNetmask;
                localMac = entryMac;
                interfaceName = networkInterface.name();
                havePrimary = true;
            }
        }
    }

    const QString targetIp = m_config->ip();
    const bool available = havePrimary;
    const int subnetStatus = available
        ? calculateSubnetStatus(sourceIp, targetIp, netmask) : 0;

    const bool sourceChanged = m_sourceIp != sourceIp;
    const bool targetChanged = m_targetIp != targetIp;
    const bool hasNetmaskChange = m_netmask != netmask;
    const bool macChanged = m_localMac != localMac;
    const bool interfaceChanged = m_interfaceName != interfaceName;
    const bool hasAvailableChange = m_available != available;
    const bool subnetChanged = m_subnetStatus != subnetStatus;
    const bool hasInterfacesChange = m_interfaces != interfaces;

    m_sourceIp = sourceIp;
    m_targetIp = targetIp;
    m_netmask = netmask;
    m_localMac = localMac;
    m_interfaceName = interfaceName;
    m_available = available;
    m_subnetStatus = subnetStatus;
    m_interfaces = interfaces;

    if (sourceChanged) emit sourceIpChanged();
    if (targetChanged) emit targetIpChanged();
    if (hasNetmaskChange) emit netmaskChanged();
    if (macChanged) emit localMacChanged();
    if (interfaceChanged) emit interfaceNameChanged();
    if (hasAvailableChange) emit availableChanged();
    if (subnetChanged) emit subnetStatusChanged();
    if (hasInterfacesChange) emit interfacesChanged();
}

void NetworkInfoController::updateTargetIp() {
    if (m_targetIp == m_config->ip()) return;
    m_targetIp = m_config->ip();
    emit targetIpChanged();
    const int status = m_available
        ? calculateSubnetStatus(m_sourceIp, m_targetIp, m_netmask) : 0;
    if (m_subnetStatus != status) {
        m_subnetStatus = status;
        emit subnetStatusChanged();
    }
}

} // namespace gdei
