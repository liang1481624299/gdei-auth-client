// src/platform/RouteBinder.hpp
// 层 B：门户 IP 主机路由强制 / Layer B: portal-IP host-route forcing.
//
// 当源地址绑定（层 A）不可用时，给门户目标 IP 加一条临时 /32 主机路由，
// 显式指定校园网卡网关与接口，让系统把门户流量送出校园网卡；现有 QNAM 链路
// 基本不改。路由项为临时项，应用退出 / 析构时清理，不写持久路由。
//
// Fallback after layer A: add a temporary /32 host route for the portal IP,
// pinned to the campus NIC gateway and interface, so the OS sends portal
// traffic over the campus adapter. Routes are temporary and removed on
// release / destruction; no persistent route is written.
//
// 边界 / Boundary:
//  - Windows 需管理员权限；无权限或失败时静默降级到层 C（仅识别标注）。
//    Windows requires elevation; without it, fail silently down to layer C.
//  - 仅作用于门户主机 / IP，不影响其他流量。
//    Only the portal host/IP is touched; other traffic is unaffected.
//  - macOS/Linux 预留；Android 不做路由改写。
//    macOS/Linux are placeholders; Android does not rewrite routes.
#ifndef GDEI_ROUTE_BINDER_HPP
#define GDEI_ROUTE_BINDER_HPP

#include <QObject>
#include <QString>
#include <QVariantMap>

namespace gdei {

// 层 B 主机路由状态 / Layer-B host-route status.
enum class RouteStatus {
    Idle,         // 未尝试 / not attempted
    Bound,        // 临时主机路由已添加 / temporary host route added
    Degraded,     // 尝试失败，已回退层 C / failed, fell back to layer C
    Unsupported   // 当前平台不支持路由改写 / platform does not support route rewrite
};

class RouteBinder : public QObject {
    Q_OBJECT
public:
    explicit RouteBinder(QObject* parent = nullptr);
    ~RouteBinder() override;

    // 依据识别出的校园网卡与门户 IP 加临时 /32 主机路由。
    // selected 为 NicSelector::selected() 结果；portalIp 为门户 IPv4。
    // Add a temporary /32 host route via the campus gateway/interface.
    void bind(const QVariantMap& selected, const QString& portalIp);

    // 清理本进程添加的临时路由（析构时自动调用）。
    // Remove temporary routes added by this process (called on destruction).
    void release();

    RouteStatus status() const { return m_status; }
    QString note() const { return m_note; }
    QString boundIp() const { return m_boundIp; }
    bool routeAdded() const { return m_routeAdded; }
    bool isElevated() const { return m_elevated; }

private:
    RouteStatus m_status = RouteStatus::Idle;
    QString m_note;
    QString m_boundIp;       // 已绑定的门户 IPv4 / portal IPv4 bound
    int m_ifIndex = -1;
    QString m_gateway;
    bool m_elevated = false;
    bool m_routeAdded = false;

#ifdef Q_OS_WIN
    static bool winIsElevated();
    QString winGatewayForIndex(int ifIndex) const;
    bool winAddRoute();
    void winDeleteRoute();
#endif
};

} // namespace gdei

#endif // GDEI_ROUTE_BINDER_HPP
