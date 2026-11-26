// src/app/NetworkInfoController.cpp
#include "app/NetworkInfoController.hpp"
#include "core/ConfigStore.hpp"
#include "core/NicSelector.hpp"
#include "platform/RouteBinder.hpp"

#include <QAbstractSocket>
#include <QFile>
#include <QHostAddress>
#include <QNetworkAddressEntry>
#include <QNetworkInterface>
#include <QStringList>
#include <QTextStream>
#include <QtEndian>
#include <QVariantMap>

#ifdef Q_OS_WIN
// 必须先包含 windows.h（经 qt_windows.h 包装），再包含 iphlpapi.h；
// 否则 MSVC 下 ifmib.h/nldef.h 中的基础类型尚未定义，会报 C3646/C2143。
// Include windows.h first, then iphlpapi.h; MSVC requires the base types
// from windows.h to be defined before the MIB headers.
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <qt_windows.h>
#include <iphlpapi.h>
#endif

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

// 同子网时从系统邻接表解析目标 MAC。只读系统缓存，不发送任何探测包；
// 目标未在邻接表中（未通信过）时返回空，由 UI 显示“未提供”。
// Read the OS neighbour table; no probes are sent. Empty means "not available".
QString lookupNeighborMac(const QString& ip) {
    if (ip.isEmpty()) return QString();

#ifdef Q_OS_WIN
    const quint32 target = QHostAddress(ip).toIPv4Address();
    if (target == 0) return QString();
    ULONG size = 0;
    if (GetIpNetTable(nullptr, &size, TRUE) != ERROR_INSUFFICIENT_BUFFER || size == 0) {
        return QString();
    }
    QByteArray buffer(static_cast<int>(size), Qt::Uninitialized);
    auto* table = reinterpret_cast<PMIB_IPNETTABLE>(buffer.data());
    if (GetIpNetTable(table, &size, TRUE) != NO_ERROR || table == nullptr) {
        return QString();
    }
    const quint32 targetNetOrder = qToBigEndian<quint32>(target);
    for (DWORD i = 0; i < table->dwNumEntries; ++i) {
        const MIB_IPNETROW& row = table->table[i];
        if (row.dwAddr != targetNetOrder) continue;
        if (row.dwPhysAddrLen < 6) continue;
        QStringList parts;
        parts.reserve(6);
        for (int b = 0; b < 6; ++b) {
            parts << QStringLiteral("%1").arg(row.bPhysAddr[b], 2, 16, QLatin1Char('0')).toUpper();
        }
        return parts.join(QLatin1Char(':'));
    }
    return QString();
#else
    // Linux / Android：/proc/net/arp（Android 部分版本受限，读不到则返回空）。
    QFile arp(QStringLiteral("/proc/net/arp"));
    if (!arp.open(QIODevice::ReadOnly | QIODevice::Text)) return QString();
    QTextStream stream(&arp);
    stream.readLine();  // 跳过表头
    while (!stream.atEnd()) {
        const QString line = stream.readLine().simplified();
        const QStringList cols = line.split(QLatin1Char(' '));
        if (cols.size() < 4) continue;
        if (cols.at(0) != ip) continue;
        const QString mac = cols.at(3).toUpper();
        if (mac == QLatin1String("00:00:00:00:00:00")) return QString();
        return mac;
    }
    return QString();
#endif
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
    : QObject(parent), m_config(config),
      m_nicSelector(new NicSelector(config, this)),
      m_routeBinder(new RouteBinder(this)) {
    connect(m_config, &ConfigStore::ipChanged, this, &NetworkInfoController::updateTargetIp);
    refresh();
}

QString NetworkInfoController::sourceIp() const { return m_sourceIp; }
QString NetworkInfoController::targetIp() const { return m_targetIp; }
QString NetworkInfoController::netmask() const { return m_netmask; }
QString NetworkInfoController::localMac() const { return m_localMac; }
QString NetworkInfoController::targetMac() const { return m_targetMac; }
QString NetworkInfoController::interfaceName() const { return m_interfaceName; }
bool NetworkInfoController::available() const { return m_available; }
int NetworkInfoController::subnetStatus() const { return m_subnetStatus; }
QVariantList NetworkInfoController::interfaces() const { return m_interfaces; }

QString NetworkInfoController::campusSourceIp() const {
    return m_campusSelected.value(QStringLiteral("sourceIp")).toString();
}
QString NetworkInfoController::campusInterfaceName() const {
    return m_campusSelected.value(QStringLiteral("interfaceName")).toString();
}
bool NetworkInfoController::campusReachable() const {
    return m_campusSelected.value(QStringLiteral("reachable")).toBool();
}
bool NetworkInfoController::campusMismatch() const {
    const QString campusIp = campusSourceIp();
    return !campusIp.isEmpty() && campusIp != m_sourceIp;
}
QVariantList NetworkInfoController::campusNicList() const { return m_campusNicList; }
QVariantMap NetworkInfoController::campusSelected() const { return m_campusSelected; }

int NetworkInfoController::routeStatus() const {
    return static_cast<int>(m_routeBinder->status());
}
QString NetworkInfoController::routeNote() const {
    return m_routeBinder->note();
}
bool NetworkInfoController::routeBound() const {
    return m_routeBinder->routeAdded();
}

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
    const QString targetMac = (available && subnetStatus == 1)
        ? lookupNeighborMac(targetIp) : QString();

    const bool sourceChanged = m_sourceIp != sourceIp;
    const bool targetChanged = m_targetIp != targetIp;
    const bool hasNetmaskChange = m_netmask != netmask;
    const bool macChanged = m_localMac != localMac;
    const bool interfaceChanged = m_interfaceName != interfaceName;
    const bool hasAvailableChange = m_available != available;
    const bool subnetChanged = m_subnetStatus != subnetStatus;
    const bool targetMacChangedFlag = m_targetMac != targetMac;
    const bool hasInterfacesChange = m_interfaces != interfaces;

    m_sourceIp = sourceIp;
    m_targetIp = targetIp;
    m_netmask = netmask;
    m_localMac = localMac;
    m_interfaceName = interfaceName;
    m_available = available;
    m_subnetStatus = subnetStatus;
    m_targetMac = targetMac;
    m_interfaces = interfaces;

    // 校园网卡识别（C 层）：逐候选绑定源地址探测门户可达性。当前仅识别标注，
    // 不修改路由、不替换门户客户端的传输连接；探测为同步短超时，暂在主线程执行。
    // Campus NIC detection (layer C): probe portal reachability per candidate by binding
    // the source address. Advisory only: no route change, no transport replacement yet.
    m_nicSelector->refresh();
    const QVariantList oldCampusList = m_campusNicList;
    const QVariantMap oldCampusSelected = m_campusSelected;
    m_campusNicList = m_nicSelector->nicList();
    m_campusSelected = m_nicSelector->selected();

    // 层 B：识别到可达校园网卡时尝试加临时主机路由；其余情况先清理上一轮路由。
    // Layer B: add a temporary host route when a reachable campus NIC is found;
    // otherwise release any route added previously.
    if (!m_campusSelected.isEmpty() &&
        m_campusSelected.value(QStringLiteral("reachable")).toBool()) {
        m_routeBinder->bind(m_campusSelected, m_config->ip());
    } else {
        m_routeBinder->release();
    }

    const bool campusHasChange = (m_campusNicList != oldCampusList) ||
                                 (m_campusSelected != oldCampusSelected) ||
                                 m_routeBinder->routeAdded();

    if (sourceChanged) emit sourceIpChanged();
    if (targetChanged) emit targetIpChanged();
    if (hasNetmaskChange) emit netmaskChanged();
    if (macChanged) emit localMacChanged();
    if (interfaceChanged) emit interfaceNameChanged();
    if (hasAvailableChange) emit availableChanged();
    if (subnetChanged) emit subnetStatusChanged();
    if (targetMacChangedFlag) emit targetMacChanged();
    if (hasInterfacesChange) emit interfacesChanged();
    if (campusHasChange) emit campusChanged();
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
    const QString mac = (m_available && status == 1) ? lookupNeighborMac(m_targetIp) : QString();
    if (m_targetMac != mac) {
        m_targetMac = mac;
        emit targetMacChanged();
    }
}

} // namespace gdei
