// src/core/NicSelector.cpp
#include "core/NicSelector.hpp"
#include "core/ConfigStore.hpp"
#include "core/Constants.hpp"

#include <QAbstractSocket>
#include <QHostAddress>
#include <QNetworkAddressEntry>
#include <QNetworkInterface>
#include <QTcpSocket>

namespace gdei {
namespace {

// 远程共享/桥接关键词（RNDIS、USB 网络共享、网桥等）。
// Remote-sharing / bridge keywords (RNDIS, USB tethering, bridge...).
const QStringList kRemoteKeywords = {
    "remote ndis", "rndis", "sharing", "usb tether", "bridge", "ppp"
};

// VPN / 隧道关键词 / VPN / tunnel keywords.
const QStringList kTunnelKeywords = {
    "tailscale", "tun", "tap", "wireguard", "openvpn", "zerotier",
    "warp", "vpn", "tunnel"
};

// 虚拟机 / 容器虚拟网卡关键词 / VM / container virtual adapter keywords.
const QStringList kVirtualKeywords = {
    "vethernet", "hyper-v", "hyperv", "vmware", "virtualbox", "vbox",
    "wsl", "docker", "veth", "vmxnet", "virtual"
};

constexpr int kProbeTimeoutMs = 3000;

QString kindText(NicKind kind) {
    switch (kind) {
    case NicKind::Physical: return QStringLiteral("physical");
    case NicKind::Virtual: return QStringLiteral("virtual");
    case NicKind::Remote:  return QStringLiteral("remote");
    case NicKind::Tunnel:  return QStringLiteral("tunnel");
    case NicKind::Loopback: return QStringLiteral("loopback");
    case NicKind::Unknown: break;
    }
    return QStringLiteral("unknown");
}

} // namespace

NicSelector::NicSelector(ConfigStore* config, QObject* parent)
    : QObject(parent), m_cfg(config) {
    refresh();
}

bool NicSelector::matchesKeyword(const QString& text, const QStringList& keywords) {
    for (const QString& kw : keywords) {
        if (text.contains(kw)) return true;
    }
    return false;
}

NicKind NicSelector::classify(const QNetworkInterface& iface, bool& excluded) {
    const QString name = iface.name().toLower();
    const QString human = iface.humanReadableName().toLower();
    const QString text = name + QLatin1Char(' ') + human;

    excluded = false;
    if (matchesKeyword(text, kRemoteKeywords)) {
        excluded = true;
        return NicKind::Remote;
    }
    if (matchesKeyword(text, kTunnelKeywords)) {
        excluded = true;
        return NicKind::Tunnel;
    }
    if (matchesKeyword(text, kVirtualKeywords)) {
        excluded = true;
        return NicKind::Virtual;
    }
    return NicKind::Physical;
}

bool NicSelector::probePortal(const QHostAddress& sourceIp) const {
    // 绑定指定源地址后直连门户，验证该网卡能否到达认证网关。
    // Bind the source address, then connect to the portal to verify reachability.
    const QString host = !m_cfg->host().isEmpty() ? m_cfg->host()
                         : !m_cfg->ip().isEmpty() ? m_cfg->ip()
                         : constants::DEFAULT_HOST;

    QTcpSocket socket;
    if (!socket.bind(sourceIp, 0, QAbstractSocket::ShareAddress)) {
        return false;
    }
    socket.connectToHost(host, constants::DEFAULT_PORT);
    const bool ok = socket.waitForConnected(kProbeTimeoutMs);
    socket.close();
    return ok;
}

void NicSelector::refresh() {
    m_nicList.clear();
    m_selected = QVariantMap();

    int bestScore = -1;
    QVariantMap best;
    bool anyProbe = false;

    const auto all = QNetworkInterface::allInterfaces();
    for (const QNetworkInterface& iface : all) {
        const auto flags = iface.flags();
        if (!(flags & QNetworkInterface::IsUp) ||
            !(flags & QNetworkInterface::IsRunning) ||
            (flags & QNetworkInterface::IsLoopBack)) {
            continue;
        }

        bool excluded = false;
        const NicKind kind = classify(iface, excluded);

        const auto entries = iface.addressEntries();
        for (const QNetworkAddressEntry& entry : entries) {
            if (entry.ip().protocol() != QAbstractSocket::IPv4Protocol ||
                entry.ip().isLoopback()) {
                continue;
            }

            const QHostAddress ip = entry.ip();
            const bool reachable = !excluded ? probePortal(ip) : false;
            if (reachable) anyProbe = true;

            int score = 0;
            if (reachable) score += 3;
            if (kind == NicKind::Physical) score += 2;
            else if (kind == NicKind::Virtual) score += 1;
            if (!iface.hardwareAddress().isEmpty()) ++score;
            if (ip.isInSubnet(QHostAddress("169.254.0.0"), 16)) score -= 10;

            QVariantMap item;
            item[QStringLiteral("name")] = iface.name();
            item[QStringLiteral("humanName")] = iface.humanReadableName();
            item[QStringLiteral("index")] = iface.index();
            item[QStringLiteral("ip")] = ip.toString();
            item[QStringLiteral("netmask")] = entry.netmask().toString();
            item[QStringLiteral("mac")] = iface.hardwareAddress();
            // 网关/DNS 暂留空：Windows 经 iphlpapi 读取在 B 层接入；其他平台留空。
            // Gateway/DNS left empty: Windows reads them via iphlpapi in layer B.
            item[QStringLiteral("gateway")] = QString();
            item[QStringLiteral("kind")] = static_cast<int>(kind);
            item[QStringLiteral("kindText")] = kindText(kind);
            item[QStringLiteral("excluded")] = excluded;
            item[QStringLiteral("reachable")] = reachable;
            item[QStringLiteral("score")] = score;
            m_nicList.append(item);

            if (!excluded && score > bestScore) {
                bestScore = score;
                best = item;
            }
        }
    }

    if (!best.isEmpty()) {
        const auto method = anyProbe
            ? NicSelectMethod::Probe : NicSelectMethod::Score;
        m_selected[QStringLiteral("sourceIp")] = best.value(QStringLiteral("ip"));
        m_selected[QStringLiteral("interfaceName")] = best.value(QStringLiteral("name"));
        m_selected[QStringLiteral("humanName")] = best.value(QStringLiteral("humanName"));
        m_selected[QStringLiteral("index")] = best.value(QStringLiteral("index"));
        m_selected[QStringLiteral("gateway")] = best.value(QStringLiteral("gateway"));
        m_selected[QStringLiteral("reachable")] = best.value(QStringLiteral("reachable"));
        m_selected[QStringLiteral("method")] = static_cast<int>(method);
        // 传输层 A/B 尚未接入前，统一落在 C（仅识别标注）。
        // Until transport layers A/B are wired, default to advisory mode C.
        m_selected[QStringLiteral("bindMode")] = static_cast<int>(NicBindMode::Advisory);
        m_selected[QStringLiteral("note")] =
            anyProbe ? QStringLiteral("portal reachable on selected NIC")
                     : QStringLiteral("no probe hit; selected by score");
    } else {
        m_selected[QStringLiteral("method")] = static_cast<int>(NicSelectMethod::None);
        m_selected[QStringLiteral("bindMode")] = static_cast<int>(NicBindMode::Advisory);
        m_selected[QStringLiteral("note")] = QStringLiteral("no eligible adapter");
    }
}

QString NicSelector::campusSourceIp() const {
    return m_selected.value(QStringLiteral("sourceIp")).toString();
}

QString NicSelector::campusInterfaceName() const {
    return m_selected.value(QStringLiteral("interfaceName")).toString();
}

} // namespace gdei
