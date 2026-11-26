import QtQuick
import QtQuick.Layouts
import "../theme"
import "../components"

// 发现页 / Discover —— 网络工具聚合入口（参考发现页分组列表编排）。
// 事件记录与系统日志入口归「我的」页；本机链路详情移至首页；本页仅接入现有真实后端工具，不伪造测速、扫描类工具。
// Discover: network tools; activity/event and log entries live in Mine; link details live in Home.
PageScaffold {
    id: root
    Theme { id: th }

    signal openDiagnosticsRequested()

    title: "nav.discover"
    subtitle: "discover.subtitle"
    showBack: false
    contentMaxWidth: th.dp(720)

    function localized(key) { return app.i18n.localize(key); }

    // ── 网络工具 ──
    BaseCard {
        Layout.fillWidth: true
        title: root.localized("discover.toolsGroup")

        NavEntryRow {
            title: root.localized("diagnostics.title")
            subtitle: root.localized("diagnostics.subtitle")
            iconName: "bolt"
            onClicked: root.openDiagnosticsRequested()
        }
    }
}
