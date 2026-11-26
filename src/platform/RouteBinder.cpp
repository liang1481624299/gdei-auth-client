// src/platform/RouteBinder.cpp
#include "platform/RouteBinder.hpp"

#include <QAbstractSocket>
#include <QHostAddress>
#include <QProcess>

#ifdef Q_OS_WIN
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
// MinGW 下 winsock2.h 必须先于 windows.h / iphlpapi.h，否则 AF_INET、
// sockaddr_in、IP_ADAPTER_ADDRESSES 等类型不可见。
// winsock2.h must precede windows.h/iphlpapi.h on MinGW for AF_INET etc.
#include <winsock2.h>
#include <qt_windows.h>
#include <iphlpapi.h>
#endif

namespace gdei {

namespace {
constexpr int kRouteTimeoutMs = 5000;
} // namespace

RouteBinder::RouteBinder(QObject* parent) : QObject(parent) {}

RouteBinder::~RouteBinder() {
    release();
}

void RouteBinder::bind(const QVariantMap& selected, const QString& portalIp) {
    // 先清理上一轮可能残留的临时路由。
    // Clean up any temporary route from the previous round first.
    release();

    QHostAddress portal;
    if (!portalIp.isEmpty() && portal.setAddress(portalIp) &&
        portal.protocol() == QAbstractSocket::IPv4Protocol) {
        m_boundIp = portal.toString();
    } else {
        m_status = RouteStatus::Degraded;
        m_note = QStringLiteral("portal address is not a resolvable IPv4");
        return;
    }

    const QString sourceIp = selected.value(QStringLiteral("sourceIp")).toString();
    m_ifIndex = selected.value(QStringLiteral("index")).toInt();
    m_gateway = selected.value(QStringLiteral("gateway")).toString();

#ifndef Q_OS_WIN
    // macOS/Linux 预留；Android 不做路由改写。
    // Placeholder on macOS/Linux; Android never rewrites routes.
    m_status = RouteStatus::Unsupported;
    m_note = QStringLiteral("route rewrite unsupported on this platform");
    return;
#else
    if (sourceIp.isEmpty() || m_ifIndex <= 0) {
        m_status = RouteStatus::Degraded;
        m_note = QStringLiteral("no eligible campus NIC selected");
        return;
    }

    m_elevated = winIsElevated();
    if (!m_elevated) {
        m_status = RouteStatus::Degraded;
        m_note = QStringLiteral("administrator privileges required to add a host route");
        return;
    }

    if (m_gateway.isEmpty()) {
        m_gateway = winGatewayForIndex(m_ifIndex);
    }
    if (m_gateway.isEmpty()) {
        m_status = RouteStatus::Degraded;
        m_note = QStringLiteral("cannot read campus NIC gateway");
        return;
    }

    if (winAddRoute()) {
        m_status = RouteStatus::Bound;
        m_note = QStringLiteral("host route added to campus NIC");
    } else {
        m_status = RouteStatus::Degraded;
        m_note = QStringLiteral("failed to add host route");
    }
#endif
}

void RouteBinder::release() {
#ifdef Q_OS_WIN
    if (m_routeAdded) {
        winDeleteRoute();
    }
#endif
    m_routeAdded = false;
    m_status = RouteStatus::Idle;
    m_note.clear();
    m_boundIp.clear();
    m_ifIndex = -1;
    m_gateway.clear();
}

#ifdef Q_OS_WIN

bool RouteBinder::winIsElevated() {
    HANDLE token = nullptr;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token)) {
        return false;
    }
    BOOL elevated = FALSE;
    DWORD len = 0;
    const BOOL ok = GetTokenInformation(token, TokenElevation, &elevated,
                                        sizeof(elevated), &len);
    CloseHandle(token);
    return ok && elevated;
}

QString RouteBinder::winGatewayForIndex(int ifIndex) const {
    ULONG size = 0;
    if (GetAdaptersAddresses(AF_INET, GAA_FLAG_INCLUDE_GATEWAYS, nullptr,
                             nullptr, &size) != ERROR_BUFFER_OVERFLOW) {
        return QString();
    }
    QByteArray buffer(static_cast<int>(size), Qt::Uninitialized);
    auto* addresses = reinterpret_cast<IP_ADAPTER_ADDRESSES*>(buffer.data());
    if (GetAdaptersAddresses(AF_INET, GAA_FLAG_INCLUDE_GATEWAYS, nullptr,
                              addresses, &size) != NO_ERROR) {
        return QString();
    }

    for (auto* adapter = addresses; adapter; adapter = adapter->Next) {
        if (static_cast<int>(adapter->IfIndex) != ifIndex) continue;
        const auto* gw = adapter->FirstGatewayAddress;
        if (!gw || !gw->Address.lpSockaddr) continue;
        if (gw->Address.lpSockaddr->sa_family != AF_INET) continue;
        const auto* sin = reinterpret_cast<const sockaddr_in*>(gw->Address.lpSockaddr);
        QHostAddress addr;
        addr.setAddress(QString::fromLatin1(inet_ntoa(sin->sin_addr)));
        if (!addr.isNull()) return addr.toString();
    }
    return QString();
}

bool RouteBinder::winAddRoute() {
    QProcess process;
    process.setProgram(QStringLiteral("route.exe"));
    // route add <portalIp> mask 255.255.255.255 <gateway> metric 1 if <idx>
    process.setArguments({
        QStringLiteral("add"), m_boundIp,
        QStringLiteral("mask"), QStringLiteral("255.255.255.255"),
        m_gateway,
        QStringLiteral("metric"), QStringLiteral("1"),
        QStringLiteral("if"), QString::number(m_ifIndex)
    });
    process.start();
    if (!process.waitForFinished(kRouteTimeoutMs)) {
        process.kill();
        return false;
    }
    if (process.exitCode() == 0) {
        m_routeAdded = true;
        return true;
    }
    return false;
}

void RouteBinder::winDeleteRoute() {
    QProcess process;
    process.setProgram(QStringLiteral("route.exe"));
    process.setArguments({
        QStringLiteral("delete"), m_boundIp,
        QStringLiteral("mask"), QStringLiteral("255.255.255.255")
    });
    process.start();
    process.waitForFinished(kRouteTimeoutMs);
}

#endif // Q_OS_WIN

} // namespace gdei
