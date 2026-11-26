// src/core/NicSelector.hpp
// 多网卡识别与门户可达性探测 / Multi-NIC detection and portal reachability probe.
// 枚举活动 IPv4 网卡，排除虚拟/隧道/远程共享接口，按“绑定源地址直连门户”的结果
// 选出校园网卡；本层只做只读探测，不修改路由、不发送认证报文。
// Enumerates active IPv4 adapters, excludes virtual/tunnel/remote-sharing interfaces,
// and picks the campus NIC by binding each source address and probing the portal.
// Read-only: no route changes, no authentication payload.
#ifndef GDEI_NIC_SELECTOR_HPP
#define GDEI_NIC_SELECTOR_HPP

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariantList>
#include <QVariantMap>

class QNetworkInterface;
class QHostAddress;

namespace gdei {

class ConfigStore;

// 网卡分类 / Adapter kind.
enum class NicKind {
    Physical,   // 物理有线/无线 / physical wired or wireless
    Virtual,    // 虚拟机/容器虚拟网卡 / VM or container virtual adapter
    Remote,     // 远程共享/桥接（RNDIS/USB 共享等）/ remote sharing bridge
    Tunnel,     // VPN/隧道 / VPN tunnel
    Loopback,   // 回环 / loopback
    Unknown
};

// 选中方式 / How the campus NIC was selected.
enum class NicSelectMethod {
    None,       // 无候选 / no candidate
    Probe,      // 探测门户可达 / portal probe succeeded
    Score       // 仅按打分兜底 / score fallback only
};

// 流量绑定模式（降级链 A→B→C）/ Traffic bind mode (fallback chain A→B→C).
enum class NicBindMode {
    Advisory,   // C：仅识别标注，不改传输 / detect & report only
    HostRoute,  // B：主机路由强制 / host-route forcing
    SourceBind  // A：socket 源地址绑定 / socket source bind
};

class NicSelector : public QObject {
    Q_OBJECT
public:
    explicit NicSelector(ConfigStore* config, QObject* parent = nullptr);

    // 重新枚举并探测 / Re-enumerate and probe.
    Q_INVOKABLE void refresh();

    QVariantList nicList() const { return m_nicList; }
    QVariantMap selected() const { return m_selected; }

    // 便捷读取：选中校园网卡的源 IPv4；无则空串。
    // Convenience: source IPv4 of the selected campus NIC; empty when none.
    QString campusSourceIp() const;
    QString campusInterfaceName() const;

private:
    ConfigStore* m_cfg;
    QVariantList m_nicList;
    QVariantMap m_selected;

    bool probePortal(const QHostAddress& sourceIp) const;
    static NicKind classify(const QNetworkInterface& iface, bool& excluded);
    static bool matchesKeyword(const QString& text, const QStringList& keywords);
};

} // namespace gdei

#endif // GDEI_NIC_SELECTOR_HPP
